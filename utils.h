/**
 * @file utils.h
 * @brief utility macros - functions
 */

#if !defined(UTILS_H_)
#define UTILS_H_

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
    
#endif
