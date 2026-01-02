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
/** @file abuf.c */


/* C89 standard */
#include <stdlib.h>
#include <string.h>

#include "abuf.h"


/* ---------------------------- GLOBAL FUNCTIONS ----------------------------- */

int ab_append(abuf_t* ab, const char* s, const size_t len) {
    char* new_b;

    if ((new_b = realloc(ab->b, ab->len + len)) == NULL)
        return 1;

    memcpy(&new_b[ab->len], s, len);
    ab->b = new_b;
    ab->len += len;
    return 0;
}


void ab_free(abuf_t* ab) {
    free(ab->b);
}
