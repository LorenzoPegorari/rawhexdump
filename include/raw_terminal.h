/* COPYRIGHT & LICENSE */

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
