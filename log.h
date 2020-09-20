/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>

#define __MODULE__ "[api.libpostal]"

typedef void (*lock_fn)(void *data, int lock);

typedef enum {
    LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
} log_level_t;

#define log_trace(...) log_log(LOG_TRACE, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __VA_ARGS__)

void log_set_level(log_level_t level);

void log_set_quiet(bool enable);

void log_log(log_level_t level, const char *fmt, ...);
