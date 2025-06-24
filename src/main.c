/* COPYRIGHT & LICENSE */


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
