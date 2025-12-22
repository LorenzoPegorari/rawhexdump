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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "file.h"
#include "raw_terminal.h"


/* -------------------- STATIC PROTOTYPES -------------------- */

/**
 * Callback function registered with atexit()
 */
static void at_exit_callback(void);


/* -------------------- MAIN -------------------- */

int main(int argc, char *argv[]) {
    int   i;
    int   ret;
    char* filename;

    atexit(at_exit_callback);

    if (argc < 2) {
        error_queue(RHD_ERROR_ARG1);
        exit(EXIT_FAILURE);
    }

    filename = NULL;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            /* TODO */
        } else {
            if (filename == NULL) {
                filename = argv[i];
            } else {
                error_queue(RHD_ERROR_ARG2);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (file_open(filename, "rb") != 0) {
        error_queue(RHD_ERROR_FILE1);
        error_queue(strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (init_term_raw_mode() != 0) {
        exit(EXIT_FAILURE);
    }

    /* Main loop */
    ret = start_term_loop();
    
    if (ret != 0) {
        /* PRINTF ERROR */
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}


/* -------------------- STATIC FUNCTIONS -------------------- */

static void at_exit_callback(void) {
    /* Close file if open */
    if (file_is_open()) {
        if (file_close() != 0) {
            fprintf(stderr, "%s\n", RHD_ERROR_FILE2);
            fprintf(stderr, "    -> %s\n", strerror(errno));
        }
    }
    /* Restore terminal initial state */
    if (is_term_in_raw_mode()) {
        disable_term_raw_mode();
    }
    /* Empty error queue and print all errors */
    error_flush();
}
