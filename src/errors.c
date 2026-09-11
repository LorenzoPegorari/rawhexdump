/*
 * MIT License
 *
 * Copyright (c) 2025-2026 Lorenzo Pegorari (@LorenzoPegorari)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/** @file errors.c */


/* C89 standard */
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"


#define RHD_ERRORS_QUEUE_MAX 64

#define RHD_ERRORS_BUFFER_LEN 512

#define RHD_ERRORS_QUEUE_INIT {0, {NULL}}


/* --------------------------- STATIC VARIABLES ---------------------------- */

static struct errors_queue_tag {
    size_t len;
    char*  messages[RHD_ERRORS_QUEUE_MAX];
} errors_queue = RHD_ERRORS_QUEUE_INIT;


/* --------------------------- GLOBAL FUNCTIONS ---------------------------- */

void error_queue(const char* args, ...) {
    va_list     ap;
    char        buf[RHD_ERRORS_BUFFER_LEN];
    int         chars_written;
    char*       final;

    /* If error queue is full, print warning and return */
    if (errors_queue.len >= RHD_ERRORS_QUEUE_MAX) {
        fprintf(stderr, "%s\n", "WARNING: Error queue is full!");
        return;
    }

    /* Format error string "buf" */
    va_start(ap, args);
    chars_written = vsnprintf(buf, sizeof(buf), args, ap);
    va_end(ap);

    /* If vsnprintf() had an error, gracefully handle it by simply skipping the message */
    if (chars_written < 0) {
        fprintf(stderr, "WARNING: vsnprintf() failed (error_queue). REASON: %s", strerror(errno));
        return;
    }

    /* Handle truncation error, if error message is longer than RHD_ERRORS_BUFFER_LEN */
    if ((size_t)chars_written >= sizeof(buf)) {
        fprintf(stderr, "WARNING: Error message truncated (error_queue).");
        buf[sizeof(buf) - 1] = '\0';
    }

    /* Copy the buffer in the final string. If an error happens, gracefully handle it
       by simply skipping the message */
    final = (char*)malloc(strlen(buf) + 1);
    if (final == NULL) {
        fprintf(stderr, "ERROR: malloc() failed (error_queue). REASON: %s", strerror(errno));
        return;
    }
    strcpy(final, buf);

    /* Queue the final string */
    errors_queue.messages[errors_queue.len++] = final;
}


void error_flush(void) {
    size_t i;

    /* Print all errors, and free them */
    for (i = 0; i < errors_queue.len; ++i) {
        fprintf(stderr, "%s\n", errors_queue.messages[i]);
        free(errors_queue.messages[i]);
    }

    errors_queue.len = 0;
}
