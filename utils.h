/**
 * @file utils.h
 * @brief utility macros - functions
 */

#if !defined(UTILS_H_)
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define DEBUG 0
#define debug_fprintf(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define SYSCALL(r, c, e) \
    if ((r = c) == -1) { perror(e); exit(errno); }                                  
    
#define CHECKNULL(r, c, e) \
    if ((r = c) == NULL) { perror(e); exit(errno); }

#define MUTEXCALL(r, c, e) \
    if ((r = c) != 0) { perror(e); }   
    
#define CHECKZERO(r, c, e) \
    if ((r = c) == 0) { perror(e); }

char *get_dir_path(char *name);
char *get_file_path(char *file_name, char *name);
    
#endif
