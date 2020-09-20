/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Source code: https://github.com/rxi/log.c
 */

/*
 * Modified by Yehor Smoliakov <yehors@ukr.net>
 *
 * Changes:
 *  removed color output, added milliseconds to time, added bool type,
 *  added typed enum for log levels, removed __FILE__ and __LINE__ definitions
 *  and some code style improvements
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#include "log.h"

static struct {
    void *data;
    lock_fn lock;
    log_level_t level;
    bool quiet;
} L;

static const char *level_names[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static void lock(void) {
    if (L.lock) {
        L.lock(L.data, 1);
    }
}

static void unlock(void) {
    if (L.lock) {
        L.lock(L.data, 0);
    }
}

void log_set_level(log_level_t level) {
    L.level = level;
}

void log_set_quiet(bool enable) {
    L.quiet = enable ? true : false;
}

void log_log(log_level_t level, const char *fmt, ...) {
    if (level < L.level) {
        return;
    }

    /* Acquire lock */
    lock();

    /* Log  */
    if (!L.quiet) {
        /* Get current time */
        struct tm lt;
        struct timeval tv;
        size_t n, bs;

        char buf[25];
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &lt);
        bs = sizeof(buf);
        n = strftime(buf, bs, "%F %T", &lt);
        if (bs > n) {
            snprintf(&buf[n], bs - n, ".%04ld", tv.tv_usec);
        }
        buf[bs - 1] = '\0';

        va_list args;
        fprintf(stdout, "%s %-5s %s ", buf, level_names[level], __MODULE__);
        va_start(args, fmt);
        vfprintf(stdout, fmt, args);
        va_end(args);
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    /* Release lock */
    unlock();
}
