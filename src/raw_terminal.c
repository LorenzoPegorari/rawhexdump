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
/** @file raw_terminal.c */


#define _XOPEN_SOURCE 700  /* Incorporating POSIX 2017 (for sigaction) */

/* C89 standard */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX standard */
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "abuf.h"
#include "errors.h"
#include "file.h"

#include "raw_terminal.h"


#define RHD_TERM_OUTPUT_FORMHEX_INIT  {RHD_TERM_OUTPUT_FORMHEX , 0, 0, file_append_formatted_hexs }
#define RHD_TERM_OUTPUT_FORMCHAR_INIT {RHD_TERM_OUTPUT_FORMCHAR, 0, 0, file_append_formatted_chars}
#define RHD_TERM_OUTPUT_CHAR_INIT     {RHD_TERM_OUTPUT_CHAR    , 0, 0, file_append_chars          }

#define RHD_TERM_OUTPUT_DEFAULT &output_formhex

#define RHD_TERM_CTRL_KEY(k) ((k) & 0x1f)

#define RHD_TERM_VT100_ERASE_LINE   "\x1b[0K"
#define RHD_TERM_VT100_CUR_TOP_LEFT "\x1b[1;1H"
#define RHD_TERM_VT100_CUR_HIDE     "\x1b[?25l"
#define RHD_TERM_VT100_CUR_SHOW     "\x1b[?25h"
#define RHD_TERM_VT100_ERASE_SCREEN "\x1b[2J"


/* -------------------------------- TYPEDEFS --------------------------------- */

/**
 * Enum type that describes the possible SIGWINCH handler states
 */
typedef enum sigwinch_state_tag {
    RHD_TERM_SIGWINCH_STATE_OK,
    RHD_TERM_SIGWINCH_STATE_ERROR
} sigwinch_state_t;

/**
 * Enum type that describes the possible terminal outputs ids
 */
typedef enum term_output_id_tag {
    RHD_TERM_OUTPUT_FORMHEX,
    RHD_TERM_OUTPUT_FORMCHAR,
    RHD_TERM_OUTPUT_CHAR
} term_output_id_t;

/**
 * Enum type that describes the possible keypress actions
 */
typedef enum keypress_tag {
    RHD_TERM_KEYPRESS_IGNORE,
    RHD_TERM_KEYPRESS_ACT,
    RHD_TERM_KEYPRESS_QUIT,
    RHD_TERM_KEYPRESS_ERROR
} keypress_t;

/**
 * Struct type that describes the possible terminal outputs
 */
typedef struct term_output_tag {
    term_output_id_t id;
    long int         pos;
    long int         row_len;
    size_t           (*file_read_func)(abuf_t*, const size_t);
} term_output_t;


/* ---------------------------- STATIC VARIABLES ----------------------------- */

/**
 * Enum that describes if the terminal was initialized or not
 */
static enum term_is_init_tag {
    RHD_TERM_INIT_FALSE,
    RHD_TERM_INIT_TRUE
} term_is_init = RHD_TERM_INIT_FALSE;

/**
 * Enum that describes if the terminal is in "term_loop()" or not
 */
static enum term_is_in_loop_tag {
    RHD_TERM_LOOP_FALSE,
    RHD_TERM_LOOP_TRUE
} term_is_in_loop = RHD_TERM_LOOP_FALSE;

/**
 * Struct that defines the formatted hex output
 */
static term_output_t output_formhex = RHD_TERM_OUTPUT_FORMHEX_INIT;

/**
 * Struct that defines the formatted char output
 */
static term_output_t output_formchar = RHD_TERM_OUTPUT_FORMCHAR_INIT;

/**
 * Struct that defines the char output
 */
static term_output_t output_char = RHD_TERM_OUTPUT_CHAR_INIT;

/**
 * Struct containing signal data (to handle SIGWINCH)
 */
static struct sigwinch_tag {
    sigwinch_state_t state;
    struct sigaction sa;
} sigwinch;

/**
 * Struct containing terminal data
 */
static struct terminal_tag {
    term_output_t* active_output;
    unsigned int   screen_rows;
    unsigned int   screen_cols;
    struct termios initial_state;  /* For preservation of initial state */
} term;


/* ---------------------------- STATIC PROTOTYPES ---------------------------- */

/**
 * Callback function registered with atexit() that handles the terminal and error messages
 */
static void at_exit_callback(void);

/**
 * Handles SIGWINCH signal, and sets term.screen_rows, term.screen_cols and sigwinch.state accordingly.
 * If successful sets sigwinch.state to RHD_TERM_SIGWINCH_STATE_OK, else RHD_TERM_SIGWINCH_STATE_ERROR.
 */
static void sigwinch_handler(int sig);

/**
 * Uses ioctl() to get terminal window size.
 * If successful returns 0, else 1.
 */
static int term_get_win_size(void);

/**
 * Saves active output.
 * If successful returns 0, else 1.
 */
static int term_output_save(void);

/**
 * Changes active output.
 * If successful returns 0, else 1.
 */
static int term_output_change(const term_output_id_t output_id);

/**
 * Sets "row_len" and "pos" of all "output_t" variables based on term window size.
 * If successful returns 0, else 1.
 */
static int term_output_adjust_after_sigwinch(void);

/**
 * Handles keypresses.
 * Returns:
 * - RHD_KEYPRESS_IGNORE = ignore keypress (no refreshing required)
 * - RHD_KEYPRESS_ACT    = keypress requires action (refreshing required)
 * - RHD_KEYPRESS_QUIT   = should exit the program
 * - RHD_KEYPRESS_ERROR  = error occurred
 */
static int term_process_keypress(void);

/**
 * Waits for key from stdin, and reads it.
 * If successful returns 0, else 1.
 */
static int term_read_key(char* c);

/**
 * Refreshes screen.
 * If successful returns 0, else 1.
 */
static int term_screen_refresh(void);

/**
 * Fills "ab" with the content of the rows to draw on screen.
 * If successful returns 0, else 1.
 */
static int term_screen_prepare_rows(abuf_t* ab);

/**
 * Clears screen.
 * If successful returns 0, else 1.
 */
static int term_screen_clear(void);


/* ---------------------------- GLOBAL FUNCTIONS ----------------------------- */

/* TERMINAL */

int term_init(const char* filename) {
    struct termios raw;

    /* If terminal is already in raw mode, return */
    if (term_is_init == RHD_TERM_INIT_TRUE) {
        error_queue("WARNING: Terminal is already initialized!");
        return 0;
    }
    
    /* Open file with given "filename" */
    if (file_open(filename, "rb") != 0) {
        fprintf(stderr, "ERROR: Could not open file!\n");
        fprintf(stderr, "    -> %s\n", strerror(errno));
        return 1;
    }

    /* Register at_exit_callback() */
    if (atexit(at_exit_callback) != 0) {
        fprintf(stderr, "ERROR: Could not set exit handler!\n");
        return 2;
    }

    /* Initialize variables */
    term.active_output = RHD_TERM_OUTPUT_DEFAULT;
    sigwinch.state     = RHD_TERM_SIGWINCH_STATE_OK;

    /* Set signal handler for SIGWINCH, and then raise it to initialize terminal window size */
    memset(&sigwinch.sa, 0, sizeof(sigwinch.sa));
    sigwinch.sa.sa_handler = sigwinch_handler;
    sigwinch.sa.sa_flags   = SA_RESTART;
    if (sigaction(SIGWINCH, &sigwinch.sa, NULL) == -1) {
        fprintf(stderr, "ERROR: Could not set sigaction for SIGWINCH!\n");
        fprintf(stderr, "    -> %s\n", strerror(errno));
        return 3;
    }
    if (raise(SIGWINCH) != 0) {
        fprintf(stderr, "ERROR: Could not raise SIGWINCH!\n");
        return 4;
    }
    if (sigwinch.state == RHD_TERM_SIGWINCH_STATE_ERROR) {
        fprintf(stderr, "ERROR: Error while handling SIGWINCH!\n");
        return 5;
    }

    /* Get terminal initial state and save it for later */
    if (tcgetattr(STDIN_FILENO, &term.initial_state) == -1) {
        fprintf(stderr, "ERROR: Could not get terminal initial state!\n");
        fprintf(stderr, "    -> %s\n", strerror(errno));
        return 6;
    }

    /* Copy initial state, and change flags to make put the terminal in raw mode */
    raw = term.initial_state;

    /*
     * BRKINT = turned off so that a break condition will not cause a SIGINT
     * ICRNL  = turned off so that Carriage Returns aren't translated into New Lines (fixes CTRL+M)
     * INLCR  = turned off so that New Lines aren't tranlsated into Carriage Returns
     * INPCK  = turned off to disable parity checking (not on modern terminals)
     * ISTRIP = turned off so the 8th bit is not stripped (this option is probably already off)
     * IXON   = turned off to disable software flow control (CTRL+S and CTRL+Q)
     */
    raw.c_iflag &= ~(BRKINT | ICRNL | INLCR | INPCK | ISTRIP | IXON);
    /* OPOST = turned off to disable all output processing features (line '\n' being translated to "\r\n") */
    raw.c_oflag &= ~(OPOST);
    /* CS8 = bit mask to set the character size to 8 bits per byte (probably already set like this) */
    raw.c_cflag |= ~(CS8);
    /*
     * ECHO   = turned off to avoid that characters in input are echoed
     * ICANON = turned off to read byte-by-byte insted of line-by-line
     * IEXTEN = turned off so to disable CTRL+V (and CTRL+0 on macOS)
     * ISIG   = turned off to avoid sending signals with CTRL+C and CTRL+Z
     */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* VMIN = value that sets the minimum amout of bytes of input needed before theread function can return */
    raw.c_cc[VMIN] = 0;
    /* VMIN = value that sets the maximum amout of time to wait before the read function can return (in tenth of a second) */
    raw.c_cc[VTIME] = 1;

    /* Set terminal in the just defined raw mode */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        fprintf(stderr, "ERROR: Could not set terminal raw state!\n");
        fprintf(stderr, "    -> %s\n", strerror(errno));
        return 7;
    }

    /* Set terminal in raw mode */
    term_is_init = RHD_TERM_INIT_TRUE;

    return 0;
}


int term_disable_raw_mode(void) {
    /* If terminal is not in raw mode, return */
    if (term_is_init == RHD_TERM_INIT_FALSE) {
        fprintf(stderr, "WARNING: Terminal was not initialized!");
        return 0;
    }

    /* Restore terminal initial state */
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term.initial_state) == -1) {
        fprintf(stderr, "ERROR: Could not set terminal initial state!");
        return 1;
    }
    
    /* Close file if open */
    if (file_close() != 0) {
        fprintf(stderr, "ERROR: Could not close opened file!\n");
        fprintf(stderr, "    -> %s\n", strerror(errno));
        return 2;
    }

    /* Flush errors queue (if they happened) */
    error_flush();

    /* Set terminal in raw mode */
    term_is_init = RHD_TERM_INIT_FALSE;

    return 0;
}


int term_loop(void) {
    int        ret;
    keypress_t keypress;

    /* Initialize variables */
    ret      = 0;
    keypress = RHD_TERM_KEYPRESS_ACT;

    term_is_in_loop = RHD_TERM_LOOP_TRUE;

    /* Hide cursor */
    if (write(STDOUT_FILENO, RHD_TERM_VT100_CUR_HIDE, sizeof(RHD_TERM_VT100_CUR_HIDE) - 1) == -1) {
        error_queue("ERROR: Function write() failed!");
        return 5;
    }

    /* Loop */
    do {
        /* Check if an eventual SIGWINCH was handled correctly */
        if (sigwinch.state == RHD_TERM_SIGWINCH_STATE_ERROR) {
            error_queue("ERROR: SIGWINCH signal was not handled correctly!");
            ret = 4;
            break;
        }

        /* If the keypress is an action, it requires a screen refresh */
        if (keypress == RHD_TERM_KEYPRESS_ACT) {
            if (term_screen_refresh() != 0) {
                error_queue("ERROR: Couldn't refresh screen!");
                ret = 2;
                break;
            }
        }

        /* Process the new keypress */
        if ((keypress = term_process_keypress()) == RHD_TERM_KEYPRESS_ERROR) {
            error_queue("ERROR: Couldn't process keypress!");
            ret = 1;
            break;
        }
    } while (keypress != RHD_TERM_KEYPRESS_QUIT && keypress != RHD_TERM_KEYPRESS_ERROR);

    if (keypress == RHD_TERM_KEYPRESS_QUIT) {
        /* If the keypress is a graceful quit, do a final screen refresh */
        if (term_screen_clear() != 0) {
            error_queue("ERROR: Couldn't clear screen!");
            ret = 3;
        }
    }

    /* Show cursor */
    if (write(STDOUT_FILENO, RHD_TERM_VT100_CUR_SHOW, sizeof(RHD_TERM_VT100_CUR_SHOW) - 1) == -1) {
        error_queue("ERROR: Function write() failed!");
        return 5;
    }

    term_is_in_loop = RHD_TERM_LOOP_FALSE;

    return ret;
}


/* ---------------------------- STATIC FUNCTIONS ----------------------------- */

/* EXIT CALLBACK */

static void at_exit_callback(void) {
    /* Restore terminal initial state */
    if (term_is_init == RHD_TERM_INIT_TRUE)
        term_disable_raw_mode();
}


/* SIGNAL HANDLER */

static void sigwinch_handler(int sig) {
    switch (sig) {
        case SIGWINCH:
            /* Adjust output */
            if (term_output_adjust_after_sigwinch() != 0) {
                sigwinch.state = RHD_TERM_SIGWINCH_STATE_ERROR;
                break;
            }

            /* Refresh screen (ONLY IF IN LOOP!) */
            if (term_is_in_loop == RHD_TERM_LOOP_TRUE) {
                if (term_screen_refresh() != 0) {
                    sigwinch.state = RHD_TERM_SIGWINCH_STATE_ERROR;
                    break;
                }
            }

            sigwinch.state = RHD_TERM_SIGWINCH_STATE_OK;
            break;

        default:
            break;
    }
}


/* TERMINAL */

static int term_get_win_size(void) {
    struct winsize ws;

    /* Tries to use ioctl() with the TIOCGWINSZ request (inside sys/ioctl.h) to get terminal window size */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0 || ws.ws_col == 0) {
        error_queue("ERROR: Function ioctl() failed!");
        return 1;
    }

    term.screen_rows = ws.ws_row;
    term.screen_cols = ws.ws_col;

    return 0;
}


/* OUTPUT */

static int term_output_save(void) {
    long int curr_pos;

    /* Get current pos of active output */
    if ((curr_pos = file_tell()) == -1) {
        error_queue("ERROR: Couldn't get current position in file!");
        return 1;
    }

    /* Save "curr_pos" inside active output */
    term.active_output->pos = curr_pos;
    
    /* Make it so that RHD_TERM_OUTPUT_FORMHEX and RHD_TERM_OUTPUT_FORMCHAR share "pos" */
    switch (term.active_output->id) {
        case RHD_TERM_OUTPUT_FORMHEX:
            output_formchar.pos = curr_pos;
            break;
        
        case RHD_TERM_OUTPUT_FORMCHAR:
            output_formhex.pos = curr_pos;
            break;

        case RHD_TERM_OUTPUT_CHAR:
            break;
        
        default:
            error_queue("ERROR: Unrecognized output id!");
            return 1;
    }

    return 0;
}


static int term_output_change(const term_output_id_t output_id) {
    /* Saves active output before changing it */
    if (term_output_save() != 0) {
        error_queue("ERROR: Couldn't save output!");
        return 1;
    }

    /* Changes active output */
    switch (output_id) {
        case RHD_TERM_OUTPUT_FORMHEX:
            term.active_output = &output_formhex;
            break;

        case RHD_TERM_OUTPUT_FORMCHAR:
            term.active_output = &output_formchar;
            break;

        case RHD_TERM_OUTPUT_CHAR:
            term.active_output = &output_char;
            break;
        
        default:
            error_queue("ERROR: Unrecognized output id!");
            return 1;
    }

    /* Move file to "pos" of new "active_output" */
    if (file_seek_set(term.active_output->pos) == 1) {
        error_queue("ERROR: Couldn't move file position indicator!");
        return 1;
    }
    
    return 0;
}


static int term_output_adjust_after_sigwinch(void) {
    /* Update terminal window size */
    if (term_get_win_size() != 0) {
        error_queue("ERROR: Couldn't get terminal window size!");
        return 1;
    }

    /* Saves output */
    if (term_output_save() != 0) {
        error_queue("ERROR: Couldn't save output!");
        return 1;
    }

    /* Update outputs "row_len" (meaning the amount of bytes that appears in a row in the term) */
    output_formhex.row_len  = term.screen_cols / 3;
    output_formchar.row_len = term.screen_cols / 3;
    output_char.row_len     = term.screen_cols;

    /* Update outputs "pos" (adjusting them based on the new "row_len") */
    output_formhex.pos  = output_formhex.pos  - (output_formhex.pos  % output_formhex.row_len );
    output_formchar.pos = output_formchar.pos - (output_formchar.pos % output_formchar.row_len);
    output_char.pos     = output_char.pos     - (output_char.pos     % output_char.row_len    );
    
    /* Move file to "pos" of "active_output" */
    if (file_seek_set(term.active_output->pos) != 0) {
        error_queue("ERROR: Couldn't move file position indicator!");
        return 1;
    }

    return 0;
}


/* INPUT */

static int term_process_keypress(void) {
    long int     row_len;
    char         c;
    unsigned int i;

    /* Read key */
    if (term_read_key(&c) != 0)
        return RHD_TERM_KEYPRESS_ERROR;

    /* Saving the "row_len" needs to happen after term_read_key().
       This is because term_read_key() is where the loop interrupts to wait for stdin.
       While it is interrupted, if the terminal window is resized, the signal SIGWINCH
       will be handled, possibly modifing all outputs' "row_len". But when the execution
       resumes, the old "row_len" will be used. */
    row_len = term.active_output->row_len;

    /* Handle keypress */
    switch (c) {
        case RHD_TERM_CTRL_KEY('q'):
            return RHD_TERM_KEYPRESS_QUIT;

        case 'w':
        case 'W':
            if (file_move(-1 * row_len) != 0)
                return RHD_TERM_KEYPRESS_ERROR;
            return RHD_TERM_KEYPRESS_ACT;

        case 's':
        case 'S':
            if (file_move(row_len) != 0)
                return RHD_TERM_KEYPRESS_ERROR;
            return RHD_TERM_KEYPRESS_ACT;

        case 'a':
        case 'A':
            if (file_move(-1 * (long int)term.screen_rows * row_len) != 0)
                return RHD_TERM_KEYPRESS_ERROR;
            return RHD_TERM_KEYPRESS_ACT;

        case 'd':
        case 'D':
            for (i = 0; i < term.screen_rows; i++) {
                if (file_move(row_len) != 0)
                    return RHD_TERM_KEYPRESS_ERROR;
            }
            return RHD_TERM_KEYPRESS_ACT;

        case 'h':
        case 'H':
            if (term.active_output->id == RHD_TERM_OUTPUT_FORMHEX)
                return RHD_TERM_KEYPRESS_IGNORE;
            if (term_output_change(RHD_TERM_OUTPUT_FORMHEX) != 0)
                return RHD_TERM_KEYPRESS_ERROR;
            return RHD_TERM_KEYPRESS_ACT;

        case 'c':
        case 'C':
            if (term.active_output->id == RHD_TERM_OUTPUT_FORMCHAR)
                return RHD_TERM_KEYPRESS_IGNORE;
            if (term_output_change(RHD_TERM_OUTPUT_FORMCHAR) != 0)
                return RHD_TERM_KEYPRESS_ERROR;
            return RHD_TERM_KEYPRESS_ACT;

        case RHD_TERM_CTRL_KEY('c'):
            if (term.active_output->id == RHD_TERM_OUTPUT_CHAR)
                return RHD_TERM_KEYPRESS_IGNORE;
            if (term_output_change(RHD_TERM_OUTPUT_CHAR) != 0)
                return RHD_TERM_KEYPRESS_ERROR;
            return RHD_TERM_KEYPRESS_ACT;
        
        default:
            return RHD_TERM_KEYPRESS_IGNORE;
    }
}


static int term_read_key(char *c) {
    ssize_t n_bytes_read;

    /* Wait for key press, and get char pressed */
    while ((n_bytes_read = read(STDIN_FILENO, c, 1)) != 1) {
        if (n_bytes_read == -1 && errno != EAGAIN) {
            error_queue("ERROR: Couldn't read keypress!");
            return 1;
        }
    }

    return 0;
}


/* OUTPUT */

static int term_screen_refresh(void) {
    abuf_t ab = ABUF_INIT;

    /* Initialize start of "ab" for screen refresh */
    if (ab_append(&ab, RHD_TERM_VT100_CUR_TOP_LEFT, sizeof(RHD_TERM_VT100_CUR_TOP_LEFT) - 1) == 1) {
        error_queue("ERROR: Function ab_append() failed!");
        return 1;
    }

    /* Initialize content of "ab" for screen refresh */
    term_screen_prepare_rows(&ab);

    /* Initialize end of "ab" for screen refresh */
    if (ab_append(&ab, RHD_TERM_VT100_CUR_TOP_LEFT, sizeof(RHD_TERM_VT100_CUR_TOP_LEFT) - 1) == 1) {
        error_queue("ERROR: Function ab_append() failed!");
        return 1;
    }

    /* Write "ab" (actual screen refresh) */
    if (write(STDOUT_FILENO, ab.b, ab.len) == -1) {
        error_queue("ERROR: Function write() failed!");
        return 1;
    }

    ab_free(&ab);

    return 0;
}


static int term_screen_prepare_rows(abuf_t* ab) {
    size_t       bytes;
    unsigned int y;

    /* Loop all rows of terminal */
    bytes = 0;
    for (y = 0; y < term.screen_rows; y++) {

        /* Fill "ab" buffer with characters read from the current row
           of the file, with the correct mode ("read_file_func") */
        bytes += term.active_output->file_read_func(ab, term.active_output->row_len);

        /* Add newline at the end, except for last row */
        if (ab_append(ab, RHD_TERM_VT100_ERASE_LINE, sizeof(RHD_TERM_VT100_ERASE_LINE) - 1) == 1) {
            error_queue("ERROR: Function ab_append() failed!");
            return 1;
        }
        if (y < term.screen_rows - 1) {
            if (ab_append(ab, "\r\n", 2) == 1) {
                error_queue("ERROR: Function ab_append() failed!");
                return 1;
            }
        }
    }

    /* Moves the file position indicator back to the beginning of the terminal page
       (meaning where the file position indicator was before calling this function) */
    if (file_move(-1 * ((long int)bytes)) != 0) {  /* DANGEROUS: converting size_t to long int */
        error_queue("ERROR: Couldn't save output!");
        return 1;
    }

    return 0;
}


static int term_screen_clear(void) {
    /* Clear screen */
    if (write(STDOUT_FILENO, RHD_TERM_VT100_ERASE_SCREEN, sizeof(RHD_TERM_VT100_ERASE_SCREEN) - 1) == -1) {
        error_queue("ERROR: Function write() failed!");
        return 1;
    }
    return 0;
}
