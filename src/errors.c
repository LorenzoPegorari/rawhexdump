/* 
 * MIT License
 * 
 * Copyright (c) 2025 Lorenzo Pegorari (@LorenzoPegorari)
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


/* C89 standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"


#define RHD_ERRORS_QUEUE_MAX 64

#define RHD_ERRORS_QUEUE_INIT {0, {NULL}}


static struct errors_queue_tag {
    unsigned int len;
    char*        messages[RHD_ERRORS_QUEUE_MAX];
} errors_queue = RHD_ERRORS_QUEUE_INIT;


void error_queue(const char* error) {
    char* str_ptr;

    if (errors_queue.len >= RHD_ERRORS_QUEUE_MAX) {
        fprintf(stderr, "%s\n", RHD_WARNING_QUEUE1);
        return;
    }

    str_ptr = (char*)malloc(strlen(error) + 1);
    if (str_ptr == NULL) {
        fprintf(stderr, "%s\n", RHD_ERROR_QUEUE1);
        exit(EXIT_FAILURE);
    }
    strcpy(str_ptr, error);

    errors_queue.messages[errors_queue.len++] = str_ptr;
}


void error_flush(void) {
    unsigned int i;

    for (i = 0; i < errors_queue.len; ++i) {
        fprintf(stderr, "%s\n", errors_queue.messages[i]);
        free(errors_queue.messages[i]);
    }

    errors_queue.len = 0;
}
