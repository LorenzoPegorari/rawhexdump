/* COPYRIGHT & LICENSE */


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
