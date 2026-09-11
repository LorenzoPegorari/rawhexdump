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
/** @file file.h */


#ifndef _FILE_H_
#define _FILE_H_


/* C89 standard */
#include <errno.h>
#include <stddef.h>

#include "abuf.h"


/**
 * Opens given "filename" file with given "modes", unless it's already open.
 * If successful returns 0, else:
 *  - 1 = error while opening given file
 *  - 2 = error while setting atexit callback
 *  - 3 = error in file position indicator
 */
int file_open(const char* filename, const char* modes);

/**
 * Closes opened file.
 * If successful returns 0, else 1.
 */
int file_close(void);

/**
 * Tries to read the given "len" amount of bytes (chars) from the file, sets "n_bytes_read"
 * to the actual amount of bytes read, and appends those bytes to the given "ab".
 * If successful returns 0, else:
 *  - 1 = error while allocating memory
 *  - 2 = error while reading given file
 *  - 3 = error during buffer append
 */
int file_append_bytes(abuf_t* ab, const size_t len, size_t* const n_bytes_read);

/**
 * Tries to read the given "len" amount of bytes (chars) from the file, sets "n_bytes_read"
 * to the actual amount of bytes read, and appends those bytes to the given "ab" in
 * hexadecimal form, with a space in between them (like this: "xx xx xx").
 * If successful returns 0, else:
 *  - 1 = error while reading given file
 *  - 2 = error while allocating memory
 *  - 3 = error during buffer append
 */
int file_append_formatted_hexs(abuf_t* ab, const size_t len, size_t* const n_bytes_read);

/**
 * Tries to read the given "len" amount of bytes (chars) from the file, sets "n_bytes_read"
 * to the actual amount of bytes read, and appends those bytes to the given "ab" in
 * ASCII form, with a space in between them (like this: " c  c  c").
 * If successful returns 0, else:
 *  - 1 = error while reading given file
 *  - 2 = error while allocating memory
 *  - 3 = error during buffer append
 */
int file_append_formatted_chars(abuf_t* ab, const size_t len, size_t* const n_bytes_read);

/**
 * Tries to read the given "len" amount of bytes (chars) from the file, sets "n_bytes_read"
 * to the actual amount of bytes read, and appends those bytes to the given "ab" in
 * ASCII form, without a space in between them (like this: "ccc").
 * If successful returns 0, else:
 *  - 1 = error while reading given file
 *  - 2 = error during buffer append
 */
int file_append_chars(abuf_t* ab, const size_t len, size_t* const n_bytes_read);

/**
 * Move file position indicator.
 * If the file position indicator would go out of the file (towards SEEK_SET), it moves to SEEK_SET.
 * If the file position indicator would go out of the file (towards SEEK_END), it doesn't move.
 * If successful returns 0, else:
 *  - 1 = error while getting file position indicator
 *  - 2 = error while setting file position indicator to beginning of file
 */
int file_move(const long int bytes);

/**
 * If file is open returns current file position, else -1
 */
long int file_tell(void);

/**
 * Move file position indicator relative from the beginning of the file.
 * If successful returns 0, else 1.
 */
int file_seek_set(const long bytes);


#endif
