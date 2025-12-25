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
/** @file file.c */


/* C89 standard */
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abuf.h"

#include "file.h"


/* Macros to convert char to hex */
#define CHAR_TO_HEX_UPPER(c) ((((c) & 0xF0) >> 4) < '\xA' ? ((((c) & 0xF0) >> 4) + '0') : ((((c) & 0xF0) >> 4) + 'A' - '\xA'))
#define CHAR_TO_HEX_LOWER(c) (((c) & 0x0F) < '\xA' ? (((c) & 0x0F) + '0') : (((c) & 0x0F) + 'A' - '\xA'))

#define RHD_FILE_INIT {RHD_FILE_STATE_CLOSE, 0, NULL}


/* -------------------------------- TYPEDEFS --------------------------------- */

/**
 * Enum that describes the state of the file
 */
typedef enum file_state_tag {
    RHD_FILE_STATE_CLOSE,
    RHD_FILE_STATE_OPEN
} file_state_t;


/* ---------------------------- STATIC VARIABLES ----------------------------- */

/**
 * Struct containing informations about the file
 */
static struct file_tag {
    file_state_t state;
    long int     len;
    FILE*        h;
} file = RHD_FILE_INIT;


/* ---------------------------- STATIC PROTOTYPES ---------------------------- */

/**
 * Callback function registered with atexit() that handles open files
 */
static void at_exit_callback(void);


/* ---------------------------- GLOBAL FUNCTIONS ----------------------------- */


/* OPEN / CLOSE */

int file_open(const char* filename, const char* modes) {
    /* If file is already open, return */
    if (file.state == RHD_FILE_STATE_OPEN)
        return 0;

    /* Open file */
    file.h = fopen(filename, modes);
    if (file.h == NULL)
        return 1;
    file.state = RHD_FILE_STATE_OPEN;

    /* Register at_exit_callback() */
    atexit(at_exit_callback);

    /* Get file length */
    if (fseek(file.h, 0, SEEK_END) == -1)
        return 2;
    if ((file.len = file_tell()) == -1)
        return 2;
    if (fseek(file.h, 0, SEEK_SET) == -1)
        return 2;

    return 0;
}


int file_close(void) {
    /* If file is already close, return */
    if (file.state == RHD_FILE_STATE_CLOSE)
        return 0;

    /* Close file */
    if (fclose(file.h) == EOF)
        return 1;
    file.state = RHD_FILE_STATE_CLOSE;

    return 0;
}


/* READ */

size_t file_append_bytes(abuf_t* ab, const size_t len) {
    char*  temp;
    size_t n_bytes_read;

    /* If given "len" is 0, return error */
    if (len == 0)
        return 0;

    /* Allocate "len" bytes to "temp" */
    if ((temp = malloc(len)) == NULL)
        return 0;
    
    /* Try to read "len" bytes and write them into "temp", and get actual "n_bytes_read" */
    if ((n_bytes_read = fread(temp, 1, len, file.h)) < len && !feof(file.h)) {
        free(temp);
        return 0;
    }

    /* Append read bytes (from "temp") to given "ab" */
    if (ab_append(ab, temp, n_bytes_read)) {
        free(temp);
        return 0;
    }
    
    free(temp);

    return n_bytes_read;
}


size_t file_append_formatted_hexs(abuf_t* ab, const size_t len) {
    size_t n_bytes_read;
    abuf_t temp = ABUF_INIT;
    char*  temp_long;
    size_t i;

    /* If given "len" is 0, return error */
    if (len == 0)
        return 0;

    /* Read "len" bytes and append them to "temp", and get actual "n_bytes_read" */
    if ((n_bytes_read = file_append_bytes(&temp, len)) == 0) {
        ab_free(&temp);
        return 0;
    }

    /* To read the bytes and convert them in hexadecimal form with spaces in-between,
       we need 3 times the amount of space, minus 1, because we don't need the last space.
       This is done in order to get the following: "bbb" = "xx xx xx" (b = byte, h = hex).
       We allocate this required space in "temp_long". */
    if ((temp_long = malloc(n_bytes_read * 3 - 1)) == NULL)
        return 0;

    /* Convert all bytes to hexadecimal, with a space in-between */
    for (i = 0; i < n_bytes_read; i++) {
        temp_long[i * 3] = CHAR_TO_HEX_UPPER(temp.b[i]);
        temp_long[i * 3 + 1] = CHAR_TO_HEX_LOWER(temp.b[i]);
        if (i < n_bytes_read - 1)
            temp_long[i * 3 + 2] = ' ';
    }

    ab_free(&temp);

    /* Append final hexadecimal string (from "temp_long") to given "ab" */
    if (ab_append(ab, temp_long, n_bytes_read * 3 - 1) == 1) {
        free(temp_long);
        return 0;
    }

    free(temp_long);
    
    return n_bytes_read;
}


size_t file_append_formatted_chars(abuf_t* ab, const size_t len) {
    size_t n_bytes_read;
    abuf_t temp = ABUF_INIT;
    char*  temp_long;
    size_t i;

    /* If given "len" is 0, return error */
    if (len == 0)
        return 0;

    /* Read "len" bytes and append them to "temp", and get actual "n_bytes_read" */
    if ((n_bytes_read = file_append_bytes(&temp, len)) == 0) {
        ab_free(&temp);
        return 0;
    }

    /* To read the bytes and convert them in ASCII form with spaces in-between,
       we need 3 times the amount of space, minus 1, because we don't need the last space.
       This is done in order to get the following: "bbb" = " c  c  c" (b = byte, c = char).
       We allocate this required space in "temp_long". */
    if ((temp_long = malloc(n_bytes_read * 3 - 1)) == NULL)
        return 0;

    /* Convert all bytes to ASCII (when readable), with a space in-between */
    for (i = 0; i < n_bytes_read; i++) {
        temp_long[i * 3] = ' ';
        if (isprint(temp.b[i]) == 0)
            temp_long[i * 3 + 1] = '.';
        else
            temp_long[i * 3 + 1] = temp.b[i];
        if (i < n_bytes_read - 1)
            temp_long[i * 3 + 2] = ' ';
    }

    ab_free(&temp);

    /* Append final ASCII string (from "temp_long") to given "ab" */
    if (ab_append(ab, temp_long, n_bytes_read * 3 - 1)) {
        free(temp_long);
        return 0;
    }

    free(temp_long);
    
    return n_bytes_read;
}


size_t file_append_chars(abuf_t* ab, const size_t len) {
    size_t n_bytes_read;
    abuf_t temp = ABUF_INIT;
    size_t i;

    /* If given "len" is 0, return error */
    if (len == 0)
        return 0;

    /* Read "len" bytes and append them to "temp", and get actual "n_bytes_read" */
    if ((n_bytes_read = file_append_bytes(&temp, len)) == 0) {
        ab_free(&temp);
        return 0;
    }

    /* Read the bytes and convert them in ASCII form */
    for (i = 0; i < n_bytes_read; i++) {
        if (isprint(temp.b[i]) == 0)
            temp.b[i] = '.';
    }

    /* Append final ASCII string (from "temp") to given "ab" */
    if (ab_append(ab, temp.b, n_bytes_read)) {
        ab_free(&temp);
        return 0;
    }

    ab_free(&temp);

    return n_bytes_read;
}


/* MOVE */

int file_move(const long int bytes) {
    long int pos;

    /* If the movement would cause the file position indicator
        to end up out of the file, do no move instead */
    if ((pos = file_tell()) == -1)
        return 1;
    if (pos + bytes >= file.len)
        return 0;

    /* Move the file position indicator */
    if (fseek(file.h, bytes, SEEK_CUR) == -1) {
        /* If an error happens, try to move to the start of the file */
        if (fseek(file.h, 0, SEEK_SET) == -1)
            return 1;
    }
    
    return 0;
}


long int file_tell(void) {
    long int pos;
    if ((pos = ftell(file.h)) < 0)
        return -1;
    return pos;
}


int file_seek_set(const long bytes) {
    if (fseek(file.h, bytes, SEEK_SET) == -1)
        return 1;
    return 0;
}


/* ---------------------------- STATIC FUNCTIONS ----------------------------- */

static void at_exit_callback(void) {
    /* Close file if open */
    if (file.state == RHD_FILE_STATE_OPEN) {
        if (file_close() != 0) {
            fprintf(stderr, "ERROR: Could not close opened file!\n");
            fprintf(stderr, "    -> %s\n", strerror(errno));
        }
    }
}
