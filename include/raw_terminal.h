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


#ifndef RHD_RAW_TERMINAL_INCLUDE
#define RHD_RAW_TERMINAL_INCLUDE


/**
 * Initialize terminal data, assigns SIGWINCH signal handler and enables raw mode
 * If successful returns 0, else:
 * - 1 = couldn't set sigaction for SIGWINCH
 * - 2 = couldn't raise SIGWINCH
 * - 3 = error while handling SIGWINCH
 * - 4 = couldn't get terminal initial state
 * - 5 = couldn't set terminal raw state
 */
int init_term_raw_mode(void);

/**
 * Disable raw mode for terminal, returning to initial state
 * If successful returns 0, else 1 
 */
int disable_term_raw_mode(void);

/**
 * If terminal is in raw mode returns 1, else 0
 */
int is_term_in_raw_mode(void);

/**
 * Enter in terminal loop
 * If successfull (exits when "quit input" received) returns 0, else:
 * - 1 = error while processing a keypress
 * - 2 = couldn't refresh the screen
 */
int start_term_loop(void);


#endif  /* RHD_RAW_TERMINAL_INCLUDE */
