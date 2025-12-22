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
/** @file errors.h */


#ifndef RHD_ERRORS_INCLUDE
#define RHD_ERRORS_INCLUDE


#define RHD_ERROR_QUEUE1 "ERROR: Failed to queue error!"

#define RHD_ERROR_ARG1   "ERROR: Arguments missing!"
#define RHD_ERROR_ARG2   "ERROR: Given too many files! (maybe an unrecognized argument was passed?)"

#define RHD_ERROR_FILE1  "ERROR: Could not open file!"
#define RHD_ERROR_FILE2  "ERROR: Could not close opened file!"
#define RHD_ERROR_FILE3  "ERROR: Given file was not recognized as an ELF file!"

#define RHD_ERROR_TERM1  "ERROR: Could not set sigaction for SIGWINCH!"
#define RHD_ERROR_TERM2  "ERROR: Could not raise SIGWINCH!"
#define RHD_ERROR_TERM3  "ERROR: Error while handling SIGWINCH!"
#define RHD_ERROR_TERM4  "ERROR: Could not get terminal initial state!"
#define RHD_ERROR_TERM5  "ERROR: Could not set terminal raw state!"
#define RHD_ERROR_TERM6  "ERROR: Could not set terminal initial state!"

#define RHD_WARNING_QUEUE1 "WARNING: Error queue is full!"

#define RHD_WARNING_TERM1  "WARNING: Terminal was not initialized!"


/**
 * Add given "error" message to queue
 */
void error_queue(const char* error);

/**
 * Print all queued error messages, emptying the queue
 */
void error_flush(void);


#endif  /* RHD_ERRORS_INCLUDE */
