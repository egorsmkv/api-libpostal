#define HTTPSERVER_IMPL

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <libpostal/libpostal.h>

#include "httpserver.h"
#include "uri_encode.h"
#include "parson.h"
#include "log.h"
#include "sds.h"
#include "consts.h"
#include "responses.h"

struct http_server_s *server;
struct http_server_s *poll_server;

bool request_target_is(struct http_request_s *request, char const *target) {
    http_string_t url = http_request_target(request);
    unsigned long len = strlen(target);
    return len == url.len && memcmp(url.buf, target, url.len) == 0;
}

void handle_request(struct http_request_s *request) {
    http_request_connection(request, HTTP_AUTOMATIC);
    struct http_response_s *response = http_response_init();

    if (request_target_is(request, "/demo")) {
        http_response_status(response, STATUS_OK);
        http_response_header(response, "Content-Type", "text/html; charset=UTF-8");

        http_response_body(response, TEST_FORM_SRC, sizeof(TEST_FORM_SRC) - 1);
    } else if (request_target_is(request, "/parse")) {

        http_string_t headers_raw = http_get_token_string(request, HS_TOK_HEADER_KEY);

        sds headers = sdsnew(headers_raw.buf);
        sds *tokens;
        sds *header_item_tokens;
        int count, count_item, j, k;
        tokens = sdssplitlen(headers, sdslen(headers), "\n", 1, &count);

        sds to_parse_key = sdsnew("X-To-Parse");

        sds parse_text = sdsempty();

        sds header_item = sdsempty();

        for (j = 0; j < count; j++) {
            header_item = sdsnew(tokens[j]);

            header_item_tokens = sdssplitlen(header_item, sdslen(header_item), ":", 1, &count_item);
            for (k = 0; k < count_item; k++) {
                sds header_value = sdstrim(sdsnew(header_item_tokens[k]), " ");

                if (sdscmp(to_parse_key, header_value) == 0) {
                    log_debug("Found X-To-Parse key");
                    parse_text = sdstrim(sdsnew(header_item_tokens[k + 1]), "\r\n ");
                }
            }

            sdsfreesplitres(header_item_tokens, count_item);
        }
        sdsfreesplitres(tokens, count);
        sdsfree(to_parse_key);
        sdsfree(header_item);

        // reply as an error if there's no file to process
        if (sdslen(parse_text) == 0) {
            http_response_status(response, STATUS_BAD_REQUEST);
            http_response_header(response, "Content-Type", "application/json");
            http_response_body(response, TASK_ERROR, sizeof(TASK_ERROR) - 1);

            http_respond(request, response);
            return;
        }

        log_debug("Text to parse (raw): %s", parse_text);

        size_t len = strlen(parse_text);
        char decoded_uri[len + 1];
        decoded_uri[0] = '\0';
        uri_decode(parse_text, sdslen(parse_text), decoded_uri);
        sds parsed_text = sdsnew(decoded_uri);

        log_debug("Text to parse (uri decoded): %s", parsed_text);

        libpostal_address_parser_options_t options = libpostal_get_address_parser_default_options();
        libpostal_address_parser_response_t *parsed = libpostal_parse_address(parsed_text, options);

        JSON_Value *root_value = json_value_init_object();
        JSON_Object *root_object = json_value_get_object(root_value);
        char *response_string = NULL;

        JSON_Value *components_value = json_value_init_object();
        JSON_Object *components_object = json_value_get_object(components_value);

        for (size_t i = 0; i < parsed->num_components; i++) {
            json_object_set_string(components_object, parsed->labels[i], parsed->components[i]);
        }

        json_object_dotset_value(root_object, "components", components_value);

        // Clean up
        libpostal_address_parser_response_destroy(parsed);

        JSON_Value *expansions_value = json_value_init_array();
        JSON_Array *expansions_object = json_value_get_array(expansions_value);

        size_t num_expansions;
        libpostal_normalize_options_t options_expansions = libpostal_get_default_options();
        char **expansions = libpostal_expand_address(parsed_text, options_expansions, &num_expansions);

        for (int i = 0; i < num_expansions; i++) {
            json_array_append_string(expansions_object, expansions[i]);
        }

        // Clean up
        libpostal_expansion_array_destroy(expansions, num_expansions);

        json_object_dotset_value(root_object, "expansions", expansions_value);

        response_string = json_serialize_to_string_pretty(root_value);

        size_t response_length = sdslen(sdsnew(response_string));

        // all is okay, we can process the file
        http_response_status(response, STATUS_OK);
        http_response_header(response, "Content-Type", "application/json");

        http_response_body(response, response_string, response_length);

        http_respond(request, response);

        log_info("Task is done");

        // Clean up
        sdsfree(parse_text);
        sdsfree(parsed_text);

        json_free_serialized_string(response_string);

        return;
    } else {
        http_response_status(response, STATUS_OK);
        http_response_header(response, "Content-Type", "application/json");
        http_response_body(response, RESPONSE_INDEX, sizeof(RESPONSE_INDEX) - 1);
    }

    http_respond(request, response);
}

void handle_sigterm(int signum) {
    log_debug("Got signal, signum: %d, str: %s", signum, strsignal(signum));

    free(server);
    free(poll_server);

    libpostal_teardown();
    libpostal_teardown_parser();
    libpostal_teardown_language_classifier();

    log_info("Exiting...");

    exit(0);
}

int main() {
    log_info("Server is starting, listening at http://127.0.0.1:8080");

    // Setup (only called once at the beginning of your program)
    if (!libpostal_setup() || !libpostal_setup_parser() || !libpostal_setup_language_classifier()) {
        log_error("libpostal not is initialized");
        exit(EXIT_FAILURE);
    }

    log_info("libpostal is initialized");

    signal(SIGTERM, handle_sigterm);

    server = http_server_init(8080, handle_request);
    poll_server = http_server_init(8081, handle_request);
    http_server_listen_poll(poll_server);
    http_server_listen(server);
}