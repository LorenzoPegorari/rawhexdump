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
/** @file main.c */


/* Two-step macro (with extra level of indirection) to allow the preprocessor
   to expand the macros before they are converted to strings */
#define RHD_MAIN_STR_HELPER(x) #x
#define RHD_MAIN_STR(x) RHD_MAIN_STR_HELPER(x)

#define RHD_MAIN_VER_MAJOR 1
#define RHD_MAIN_VER_MINOR 0
#define RHD_MAIN_VER_PATCH 0

#define RHD_MAIN_VER RHD_MAIN_STR(RHD_MAIN_VER_MAJOR) "." RHD_MAIN_STR(RHD_MAIN_VER_MINOR) "." RHD_MAIN_STR(RHD_MAIN_VER_PATCH)


/* C89 standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"
#include "raw_terminal.h"


/* --------------------------- STATIC PROTOTYPES --------------------------- */

/**
 * Callback function registered with atexit() that flushes error messages
 */
static void at_exit_callback(void);


/* --------------------------------- MAIN ---------------------------------- */

int main(int argc, char* argv[]) {
    char* filename;
    int   i;

    /* Register at_exit_callback(). If this fails, we must immediately use fprintf to print
       an error, and exit (because we don't have access to errors.h functions) */
    if (atexit(at_exit_callback) != 0) {
        fprintf(stderr, "ERROR: Could not set exit handler (main).\n");
        exit(EXIT_FAILURE);
    }

    /* If no arguments were given, exit */
    if (argc < 2) {
        printf("ERROR: Arguments missing!\n");
        error_queue("Usage: %s [-v | --version] [-h | --help] <file-path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Handle arguments */
    filename = NULL;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: %s [-v | --version] [-h | --help] <file-path>\n", argv[0]);
            printf("\nUsable commands:\n");
            printf("         W = move up one row\n");
            printf("         S = move down one row\n");
            printf("         A = move up one page\n");
            printf("         D = move down one page\n");
            printf("         H = hexadecimal view (linked to char view)\n");
            printf("         C = char view (linked to hexadecimal view)\n");
            printf("    CTRL+C = compacted char view\n");
            printf("    CTRL+Q = quit\n");
            exit(EXIT_SUCCESS);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("%s version %s\n", argv[0], RHD_MAIN_VER);
            exit(EXIT_SUCCESS);
        } else {
            if (filename == NULL) {
                filename = argv[i];
            } else {
                error_queue("ERROR: Given too many files or given an unrecognized argument (main).\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Initialize terminal */
    if (term_init(filename) != 0)
        exit(EXIT_FAILURE);

    /* Main terminal loop */
    if (term_loop() != 0)
        exit(EXIT_FAILURE);

    /* Disable terminal raw mode */
    if (term_disable_raw_mode() != 0)
        exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}


/* --------------------------- STATIC FUNCTIONS ---------------------------- */

static void at_exit_callback(void) {
    /* Flush errors queue (if they happened) */
    error_flush();
}
