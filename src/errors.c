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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"


#define RHD_ERRORS_QUEUE_MAX 64

#define RHD_ERRORS_BUFFER_LEN 512

#define RHD_ERRORS_QUEUE_INIT {0, {NULL}}


/* ---------------------------- STATIC VARIABLES ----------------------------- */

static struct errors_queue_tag {
    size_t len;
    char*  messages[RHD_ERRORS_QUEUE_MAX];
} errors_queue = RHD_ERRORS_QUEUE_INIT;


/* ---------------------------- GLOBAL FUNCTIONS ----------------------------- */

void error_queue(const char* args, ...) {
    va_list     ap;
    char        buf[RHD_ERRORS_BUFFER_LEN];
    const char* format;
    char*       final;
    size_t      format_len;
    size_t      final_len;

    /* If error queue is full, print warning and return */
    if (errors_queue.len >= RHD_ERRORS_QUEUE_MAX) {
        fprintf(stderr, "%s\n", "WARNING: Error queue is full!");
        return;
    }

    /* Initialize args */
    va_start(ap, args);
    format = (const char*)args;
    args = va_arg(ap, const char*);

    /* Read all chars of format, and copy them in buf, substituting all instances
       of "%s" with the correct arg (in order) */
    format_len = 0;
    final_len = 0;
    while (format_len < strlen(format)) {
        if (format[format_len] == '%' && format[format_len + 1] == 's') {
            /* Substitute all "%s" occurrences with the correct arg (in order).
                If the are more args than "%s", the excess args are ignored.
                If the are more "%s" than args, an error is raised. */
            if (args != NULL) {
                strcpy(&(buf[final_len]), args);
                final_len += strlen(args);
                args = va_arg(ap, const char*);
                format_len += 2;
            } else {
                fprintf(stderr, "%s\n", "ERROR: Error in errors_queue() arguments!");
                exit(EXIT_FAILURE);
            }
        } else {
            /* Simply copy all other chars that are not "%s" */
            buf[final_len++] = format[format_len++];
        }
    }
    buf[final_len++] = '\0';

    /* Cleanup */
    va_end(ap);

    /* Copy the buffer in the final string */
    final = (char*)malloc(final_len);
    if (final == NULL) {
        fprintf(stderr, "%s\n", "ERROR: Failed to queue error!");
        exit(EXIT_FAILURE);
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
