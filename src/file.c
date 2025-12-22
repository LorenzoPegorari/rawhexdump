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
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abuf.h"
#include "errors.h"

#include "file.h"


#define CHAR_TO_HEX_UPPER(c) ((((c) & 0xF0) >> 4) < '\xA' ? ((((c) & 0xF0) >> 4) + '0') : ((((c) & 0xF0) >> 4) + 'A' - '\xA'))
#define CHAR_TO_HEX_LOWER(c) (((c) & 0x0F) < '\xA' ? (((c) & 0x0F) + '0') : (((c) & 0x0F) + 'A' - '\xA'))

#define RHD_FILE_INIT {RHD_FILE_STATE_CLOSE, 0, NULL}


/* -------------------- TYPEDEFS -------------------- */

/**
 * Enum that describes the state of the file
 */
typedef enum file_state_tag {
    RHD_FILE_STATE_CLOSE,
    RHD_FILE_STATE_OPEN
} file_state_t;


/* -------------------- STATIC VARIABLES -------------------- */

/**
 * Struct containing informations about the file
 */
static struct file_tag {
    file_state_t state;
    long int     len;
    FILE*        h;
} file = RHD_FILE_INIT;


/* -------------------- GLOBAL FUNCTIONS -------------------- */

/* OPEN / CLOSE / GETTERS */

unsigned char file_open(const char *filename, const char *modes) {
    if (file.state == RHD_FILE_STATE_OPEN) {
        return 1;
    }

    file.h = fopen(filename, modes);
    if (file.h == NULL) {
        return 1;
    }
    file.state = RHD_FILE_STATE_OPEN;

    if (fseek(file.h, 0, SEEK_END) == -1)
        return 1;
    if ((file.len = file_tell()) == -1)
        return 1;
    if (fseek(file.h, 0, SEEK_SET) == -1)
        return 1;

    return 0;
}

unsigned char file_close(void) {
    if (fclose(file.h) == EOF)
        return 1;
    file.state = RHD_FILE_STATE_CLOSE;
    return 0;
}

unsigned char file_is_open(void) {
    return (file.state == RHD_FILE_STATE_OPEN) ? 1 : 0;
}

/* READ */

size_t file_append_bytes(abuf_t *ab, const size_t len) {
    size_t n_bytes_read;
    char *temp;

    if ((temp = malloc(len)) == NULL)
        return 0;
    
    if ((n_bytes_read = fread(temp, 1, len, file.h)) < len && !feof(file.h)) {
        free(temp);
        return 0;
    }

    if (ab_append(ab, temp, n_bytes_read)) {
        free(temp);
        return 0;
    }
    
    free(temp);

    return n_bytes_read;
}

size_t file_append_hexs(abuf_t *ab, const size_t len) {
    unsigned int i;
    size_t n_chars_read;
    abuf_t temp = ABUF_INIT;
    char *temp_long;

    n_chars_read = file_append_bytes(&temp, len);
    if (n_chars_read == 0) {
        ab_free(&temp);
        return 0;
    }

    if ((temp_long = malloc(n_chars_read * 3 - 1)) == NULL)
        return 0;

    for (i = 0; i < n_chars_read; i++) {
        temp_long[i * 3] = CHAR_TO_HEX_UPPER(temp.b[i]);
        temp_long[i * 3 + 1] = CHAR_TO_HEX_LOWER(temp.b[i]);
        if (i < n_chars_read - 1)
            temp_long[i * 3 + 2] = ' ';
    }

    ab_free(&temp);

    if (ab_append(ab, temp_long, n_chars_read * 3 - 1) == 1) {
        free(temp_long);
        return 0;
    }

    free(temp_long);
    
    return n_chars_read;
}

size_t file_append_formatted_chars(abuf_t *ab, const size_t len) {
    unsigned int i;
    size_t n_chars_read;
    abuf_t temp = ABUF_INIT;
    char *temp_long;

    n_chars_read = file_append_bytes(&temp, len);
    if (n_chars_read == 0) {
        ab_free(&temp);
        return 0;
    }

    if ((temp_long = malloc(n_chars_read * 3 - 1)) == NULL)
        return 0;

    for (i = 0; i < n_chars_read; i++) {
        temp_long[i * 3] = ' ';
        if (isprint(temp.b[i]) == 0)
            temp_long[i * 3 + 1] = '.';
        else
            temp_long[i * 3 + 1] = temp.b[i];
        if (i < n_chars_read - 1)
            temp_long[i * 3 + 2] = ' ';
    }

    ab_free(&temp);

    if (ab_append(ab, temp_long, n_chars_read * 3 - 1)) {
        free(temp_long);
        return 0;
    }

    free(temp_long);
    
    return n_chars_read;
}

size_t file_append_chars(abuf_t *ab, const size_t len) {
    unsigned int i;
    size_t n_chars_read;
    abuf_t temp = ABUF_INIT;

    n_chars_read = file_append_bytes(&temp, len);
    if (n_chars_read == 0) {
        ab_free(&temp);
        return 0;
    }

    for (i = 0; i < n_chars_read; i++) {
        if (isprint(temp.b[i]) == 0)
            temp.b[i] = '.';
    }

    if (ab_append(ab, temp.b, n_chars_read)) {
        ab_free(&temp);
        return 0;
    }

    ab_free(&temp);
    return n_chars_read;
}

/* MOVE */

unsigned char file_move(const long int bytes) {
    if (fseek(file.h, bytes, SEEK_CUR) == -1) {
        if (fseek(file.h, 0, SEEK_SET) == -1)
            return 1;
        return 0;
    }
    return 0;
}

unsigned char file_will_be_end(const long int bytes) {
    long int pos;
    unsigned char will_be_end;

    if (fseek(file.h, bytes, SEEK_CUR) == -1)
        return 2;

    if ((pos = file_tell()) == -1)
        return 2;
    if (pos < file.len)
        will_be_end = 0;
    else
        will_be_end = 1;
    
    if (fseek(file.h, -1 * bytes, SEEK_CUR) == -1)
        return 2;

    return will_be_end;
}

long int file_tell(void) {
    long int pos;
    if ((pos = ftell(file.h)) < 0)
        return -1;
    return pos;
}

unsigned char file_seek_set(const long bytes) {
    if (fseek(file.h, bytes, SEEK_SET) == -1)
        return 1;
    return 0;
}

/* TAbLE */

/*unsigned char generate_elf_table(const char *__filename, const char *__modes) {
    unsigned char buf[256];
    
    elf_table.h = fopen(__filename, __modes);
    if (elf_table.h == NULL)
        return 1;

    if (fread(buf, 1, 4, file.h) != 4)
        return 1;
    if ((memcmp(buf, "\x7f\x45\x4c\46", 4)))
        return 1;
    if (ab_append(&elf_table.text, STR000, sizeof(STR000) - 1))
        return 1;
    
    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (buf[0] < 2)
        return 1;
    if (ab_append(&elf_table.text, STR001[buf[0]], sizeof(STR001[buf[0]]) - 1))
        return 1;

    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (buf[0] < 2)
        return 1;
    if (ab_append(&elf_table.text, STR002[buf[0]], sizeof(STR002[buf[0]]) - 1))
        return 1;

    if (fread(buf, 1, 4, file.h) != 4)
        return 1;
    if (buf[0] != '\x01')
        return 1;
    if (ab_append(&elf_table.text, STR003, sizeof(STR003) - 1))
        return 1;
    
    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (buf[0] < 13)
        return 1;
    if (ab_append(&elf_table.text, STR004[buf[0]], sizeof(STR004[buf[0]]) - 1))
        return 1;
    
    if (fread(buf, 1, 1, file.h) != 1)
        return 1;
    if (ab_append(&elf_table.text, STR005, sizeof(STR005) - 1))
        return 1;

    if (fread(buf, 1, 7, file.h) != 7)
        return 1;
    if ((memcmp(buf, "\x00\x00\x00\x00\x00\x00\x00", 7)))
        return 1;
    if (ab_append(&elf_table.text, STR006, sizeof(STR006) - 1))
        return 1;
    return 0;
}*/
