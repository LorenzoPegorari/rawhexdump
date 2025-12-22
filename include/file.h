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
/** @file file.h */


#ifndef _FILE_H_
#define _FILE_H_


/* C89 standard */
#include <stddef.h>

#include "abuf.h"


/* 
 * Opens specified file with specified mode and sets is_file_open
 * If successful returns 0, else 1
 */
unsigned char file_open(const char* filename, const char* modes);

/* 
 * Closes opened file
 * If successful returns 0, else 1
 */
unsigned char file_close(void);

/* If file is open returns 1, else 0 */
unsigned char file_is_open(void);

/* 
 * Closes opened file
 * If successful returns 0, else 1
 */
size_t file_append_bytes(abuf_t *ab, const size_t len);

size_t file_append_hexs(abuf_t *ab, const size_t len);

size_t file_append_formatted_chars(abuf_t *ab, const size_t len);

size_t file_append_chars(abuf_t *ab, const size_t len);

unsigned char file_move(const long int bytes);

unsigned char file_will_be_end(const long int bytes);

long int file_tell(void);

unsigned char file_seek_set(const long bytes);

/*unsigned char generate_elf_table(const char *__filename, const char *__modes);*/

#endif
