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


#define RHD_OUTPUT_HEX_INIT      {RHD_OUTPUT_HEX      , 0, 0, file_append_hexs           }
#define RHD_OUTPUT_FORMCHAR_INIT {RHD_OUTPUT_FORM_CHAR, 0, 0, file_append_formatted_chars}
#define RHD_OUTPUT_CHAR_INIT     {RHD_OUTPUT_CHAR     , 0, 0, file_append_chars          }

#define RHD_OUTPUT_DEFAULT &output_hex

#define RHD_CTRL_KEY(k) ((k) & 0x1f)

#define RHD_VT100_ERASE_LINE "\x1b[0K"
#define RHD_VT100_CUR_TOP_L  "\x1b[1;1H"
#define RHD_VT100_CUR_HIDE   "\x1b[?25l"
#define RHD_VT100_CUR_SHOW   "\x1b[?25h"


/* -------------------------------- TYPEDEFS --------------------------------- */

/**
 * Enum type that describes the SIGWINCH handler state
 */
typedef enum sigwinch_state_tag {
    RHD_SIGWINCH_STATE_OK,
    RHD_SIGWINCH_STATE_ERROR
} sigwinch_state_t;

/**
 * Enum type that describes the terminal mode
 */
typedef enum term_mode_tag {
    RHD_TERM_MODE_INIT_STATE,
    RHD_TERM_MODE_RAW
} term_mode_t;

/**
 * Enum type that describes the possible outputs names
 */
typedef enum output_name_tag {
    RHD_OUTPUT_HEX,
    RHD_OUTPUT_FORM_CHAR,
    RHD_OUTPUT_CHAR
} output_name_t;

/**
 * Enum type that describes the possible keypress actions
 */
typedef enum keypress_tag {
    RHD_KEYPRESS_IGNORE,
    RHD_KEYPRESS_ACT,
    RHD_KEYPRESS_QUIT,
    RHD_KEYPRESS_ERROR
} keypress_t;

/**
 * Struct type that describes an output mode
 */
typedef struct output_tag {
    output_name_t name;
    long int      pos;
    long int      row_len;
    size_t        (*write_func)(abuf_t*, const size_t);
} output_t;


/* ---------------------------- STATIC VARIABLES ----------------------------- */

/**
 * Struct that defines the hex output
 */
static output_t output_hex = RHD_OUTPUT_HEX_INIT;

/**
 * Struct that defines the formatted char output
 */
static output_t output_formatted_char = RHD_OUTPUT_FORMCHAR_INIT;

/**
 * Struct that defines the char output
 */
static output_t output_char = RHD_OUTPUT_CHAR_INIT;

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
    term_mode_t    mode;
    output_t*      active_output;
    unsigned int   screen_rows;
    unsigned int   screen_cols;
    unsigned int   cols_diff;
    struct termios initial_state;  /* For preservation of initial state */
} term;


/* ---------------------------- STATIC PROTOTYPES ---------------------------- */

/* 
 * Handles SIGWINCH signal, and sets term.screenRows, term.screenCols and sigwinch.state accordingly
 * If successful sets sigwinch.state to 0, else 1
 */
static void sigwinch_handler(int sig);

/* 
 * Uses ioctl() to get terminal window size
 * If successful returns 0, else 1
 */
static int get_term_win_size(void);

/* 
 * Saves active output
 * If successful returns 0, else 1
 */
static int save_output(void);

/* 
 * Changes active output
 * If successful returns 0, else 1
 */
static int change_output(const output_name_t output_name);

/**
 * Sets rowLen and pos of output_t variables based on term window size
 * If successful returns 0, else 1
 */
static int adjust_outputs_after_term_win_resize(void);

/* 
 * Handles inputs
 * Returns:
 * - RHD_KEYPRESS_IGNORE = no refreshing required
 * - RHD_KEYPRESS_ACT    = refreshing required
 * - RHD_KEYPRESS_QUIT   = should exit the program
 * - RHD_KEYPRESS_ERROR  = error occurred
 */
static int process_keypress(void);

/* 
 * Reads key from stdin
 * If successful returns 0, else 1
 */
static int read_key(char *c);

static int refresh_screen(void);

static int draw_rows(abuf_t *ab);


/* ---------------------------- GLOBAL FUNCTIONS ----------------------------- */

/* TERMINAL */

int init_term_raw_mode(void) {
    struct termios raw;

    /* Initialize */
    term.mode          = RHD_TERM_MODE_INIT_STATE;
    term.active_output = RHD_OUTPUT_DEFAULT;

    sigwinch.state = RHD_SIGWINCH_STATE_OK;

    /* Set signal handler for SIGWINCH, and then raise it to initialize terminal window size */
    memset(&sigwinch.sa, 0, sizeof(sigwinch.sa));
    sigwinch.sa.sa_handler = sigwinch_handler;
    sigwinch.sa.sa_flags   = SA_RESTART;
    if (sigaction(SIGWINCH, &sigwinch.sa, NULL) == -1) {
        error_queue(RHD_ERROR_TERM1);
        return 1;
    }
    if (raise(SIGWINCH)) {
        error_queue(RHD_ERROR_TERM2);
        return 2;
    }
    if (sigwinch.state == RHD_SIGWINCH_STATE_ERROR) {
        error_queue(RHD_ERROR_TERM3);
        return 3;
    }

    /* Get terminal initial state and save it for later */
    if (tcgetattr(STDIN_FILENO, &term.initial_state) == -1) {
        error_queue(RHD_ERROR_TERM4);
        return 4;
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
        error_queue(RHD_ERROR_TERM5);
        return 5;
    }

    /* Set terminal in raw mode */
    term.mode = RHD_TERM_MODE_RAW;

    return 0;
}


int disable_term_raw_mode(void) {
    if (term.mode == RHD_TERM_MODE_INIT_STATE) {
        error_queue(RHD_WARNING_TERM1);
        return 0;
    }

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &term.initial_state) == -1) {
        error_queue(RHD_ERROR_TERM6);
        return 1;
    }
    term.mode = RHD_TERM_MODE_INIT_STATE;

    return 0;
}


int is_term_in_raw_mode(void) {
    return (term.mode == RHD_TERM_MODE_RAW) ? 1 : 0;
}


int start_term_loop(void) {
    int        ret;
    keypress_t keypress;

    ret      = 0;
    keypress = RHD_KEYPRESS_ACT;

    do {
        /* TODO */
        if (keypress == RHD_KEYPRESS_ACT) {
            if (refresh_screen() != 0) {
                /* PRINTF ERROR */
                keypress = RHD_KEYPRESS_ERROR;
                ret = 2;
            }
        }

        /* TODO */
        if ((keypress = process_keypress()) == RHD_KEYPRESS_ERROR) {
            /* PRINTF ERROR */
            keypress = RHD_KEYPRESS_ERROR;
            ret = 1;
        }
    } while (keypress == RHD_KEYPRESS_ACT || keypress == RHD_KEYPRESS_IGNORE);

    term.active_output = NULL;

    if (keypress == RHD_KEYPRESS_QUIT) {
        /* TODO */
        if (refresh_screen() != 0) {
            /* PRINTF ERROR */
            keypress = RHD_KEYPRESS_ERROR;
            ret = 2;
        }
    }

    return ret;
}


/* ---------------------------- STATIC FUNCTIONS ----------------------------- */

/* SIGNAL */

static void sigwinch_handler(int sig) {
    switch (sig) {
        case SIGWINCH:
            if (adjust_outputs_after_term_win_resize() != 0)
                sigwinch.state = RHD_SIGWINCH_STATE_ERROR;
            else
                sigwinch.state = RHD_SIGWINCH_STATE_OK;
            break;

        default:
            break;
    }
}


/* TERMINAL */

static int get_term_win_size(void) {
    struct winsize ws;

    /* Tries to use ioctl() with the TIOCGWINSZ request (inside sys/ioctl.h) to get terminal window size */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0 || ws.ws_col == 0)
        return 1;

    term.screen_rows = ws.ws_row;
    term.screen_cols = ws.ws_col;
    return 0;
}


static int save_output(void) {
    long int curr_pos;

    /* Get current pos of active mode */
    if ((curr_pos = file_tell()) == -1)
        return 1;

    /* Save curr_pos inside active mode (makes it so that RHD_OUTPUT_HEX and RHD_OUTPUT_FORM_CHAR share pos) */
    switch (term.active_output->name) {
        case RHD_OUTPUT_HEX:
            output_formatted_char.pos = curr_pos;
            break;
        case RHD_OUTPUT_FORM_CHAR:
            output_hex.pos = curr_pos;
            break;

        case RHD_OUTPUT_CHAR:
            break;
        
        default:
            return 1;
    }
    term.active_output->pos = curr_pos;

    return 0;
}


static int change_output(const output_name_t output_name) {
    if (save_output() != 0)
        return 1;

    /* Changes active mode */
    switch (output_name) {
        case RHD_OUTPUT_HEX:
            term.active_output = &output_hex;
            break;

        case RHD_OUTPUT_CHAR:
            term.active_output = &output_char;
            break;

        case RHD_OUTPUT_FORM_CHAR:
            term.active_output = &output_formatted_char;
            break;
        
        default:
            return 1;
    }

    if (file_seek_set(term.active_output->pos) == 1)
        return 1;
    
    return 0;
}


static int adjust_outputs_after_term_win_resize(void) {
    if (get_term_win_size() != 0)
        return 1;

    if (save_output() != 0)
        return 1;

    output_hex.row_len            = term.screen_cols / 3;
    output_formatted_char.row_len = term.screen_cols / 3;
    output_char.row_len           = term.screen_cols;

    output_hex.pos            = (output_hex.pos            / output_hex.row_len           ) * output_hex.row_len;
    output_formatted_char.pos = (output_formatted_char.pos / output_formatted_char.row_len) * output_formatted_char.row_len;
    output_char.pos           = (output_char.pos           / output_char.row_len          ) * output_char.row_len;
    
    if (file_seek_set(term.active_output->pos) != 0)
        return 1;

    if (refresh_screen() != 0) {
        return 1;
    }

    return 0;
}


/* INPUT */

static int process_keypress(void) {
    unsigned int i;
    long int     row_len;
    char         c;

    row_len = term.active_output->row_len;

    if (read_key(&c) != 0)
        return RHD_KEYPRESS_ERROR;

    switch (c) {
        case RHD_CTRL_KEY('q'):
            return RHD_KEYPRESS_QUIT;

        case 'w':
        case 'W':
            if (file_move(-1 * row_len) == 1)
                return RHD_KEYPRESS_ERROR;
            return RHD_KEYPRESS_ACT;

        case 's':
        case 'S':
            if (file_will_be_end(row_len) == 0) {
                if (file_move(row_len) == 1)
                    return RHD_KEYPRESS_ERROR;
                return RHD_KEYPRESS_ACT;
            } else if (file_will_be_end(row_len) == 1)
                return RHD_KEYPRESS_ACT;
            else
                return RHD_KEYPRESS_ERROR;

        case 'a':
        case 'A':
            if (file_move(-1 * (long int)term.screen_rows * row_len) == 1)
                return RHD_KEYPRESS_ERROR;
            return RHD_KEYPRESS_ACT;

        case 'd':
        case 'D':
            for (i = 0; i < term.screen_rows; i++) {
                if (file_will_be_end(row_len) == 0) {
                    if (file_move(row_len) == 1)
                        return RHD_KEYPRESS_ERROR;
                } else if (file_will_be_end(row_len) == 1)
                    return RHD_KEYPRESS_ACT;
                else
                    return RHD_KEYPRESS_ERROR;
            }
            return RHD_KEYPRESS_ACT;

        case 'h':
        case 'H':
            if (term.active_output->name == RHD_OUTPUT_HEX)
                return RHD_KEYPRESS_IGNORE;
            if (change_output(RHD_OUTPUT_HEX) == 1)
                return RHD_KEYPRESS_ERROR;
            return RHD_KEYPRESS_ACT;

        case 'c':
        case 'C':
            if (term.active_output->name == RHD_OUTPUT_FORM_CHAR)
                return RHD_KEYPRESS_IGNORE;
            if (change_output(RHD_OUTPUT_FORM_CHAR) == 1)
                return RHD_KEYPRESS_ERROR;
            return RHD_KEYPRESS_ACT;

        case RHD_CTRL_KEY('c'):
            if (term.active_output->name == RHD_OUTPUT_CHAR)
                return RHD_KEYPRESS_IGNORE;
            if (change_output(RHD_OUTPUT_CHAR) == 1)
                return RHD_KEYPRESS_ERROR;
            return RHD_KEYPRESS_ACT;
        
        default:
            return RHD_KEYPRESS_IGNORE;
    }
}


static int read_key(char *c) {
    ssize_t nread;

    while ((nread = read(STDIN_FILENO, c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            return 1;
    }

    return 0;
}


/* OUTPUT */

static int refresh_screen(void) {
    abuf_t ab = ABUF_INIT;

    if (ab_append(&ab, RHD_VT100_CUR_HIDE, sizeof(RHD_VT100_CUR_HIDE) - 1) == 1)
        return 1;
    if (ab_append(&ab, RHD_VT100_CUR_TOP_L, sizeof(RHD_VT100_CUR_TOP_L) - 1) == 1)
        return 1;
    draw_rows(&ab);
    if (ab_append(&ab, RHD_VT100_CUR_TOP_L, sizeof(RHD_VT100_CUR_TOP_L) - 1) == 1)
        return 1;
    if (ab_append(&ab, RHD_VT100_CUR_SHOW, sizeof(RHD_VT100_CUR_SHOW) - 1) == 1)
        return 1;

    if (write(STDOUT_FILENO, ab.b, ab.len) == -1)
        return 1;

    ab_free(&ab);
    return 0;
}


static int draw_rows(abuf_t* ab) {
    size_t bytes;
    unsigned int y;

    bytes = 0;
    for (y = 0; y < term.screen_rows; y++) {

        if (term.active_output != NULL)
            bytes += term.active_output->write_func(ab, term.active_output->row_len);

        if (ab_append(ab, RHD_VT100_ERASE_LINE, sizeof(RHD_VT100_ERASE_LINE) - 1) == 1)
            return 1;
        if (y < term.screen_rows - 1) {
            if (ab_append(ab, "\r\n", 2) == 1)
                return 1;
        }
    }

    if (term.active_output != NULL) {
        if (file_move(-1 * ((long int)bytes)) == 0)  /* DANGEROUS: converting size_t to long int */
            return 1;
    }
    return 0;
}
