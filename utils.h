/**
 *@file utils.h
 *@brief utility macros - functions
 */

#if !defined(UTILS_H_)
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define ESOCKET     "Error on socket"
#define EREAD       "Error SYSCALL read"
#define EWRITE      "Error SYSCALL write"
#define ESCANF      "Error scanf"
#define EMALLOC     "Error malloc"
#define EOPEN       "Error opening the file"
#define EDIR        "Error creating directory"

#define DEBUG 0
#define debug_fprintf(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define SYSCALL(r, c, e) \
    if ((r = c) == -1) { perror(e); exit(errno); }                                  
    
#define CHECKNULL(r, c, e) \
    if ((r = c) == NULL) { perror(e); exit(errno); }

#define CHECKZERO(r, c, e) \
    if ((r = c) == 0) { perror(e); }
    
#define MUTEXCALL(c, e) \
    if (c != 0) { perror(e); exit(errno); }

#define CHECKEOF(c, e) \
    if (c == EOF) { perror(e); }
    
/**
 *@function get_dir_path
 *@brief get the path of the directory
 *@param name of the directory
 *@return NULL if name = NULL else the directory's path  
 */
char *get_dir_path(char *name);

/**
 *@function get_file_path
 *@brief get the path of the file in a directory
 *@param file_name is the file's name
 *@param name is the directory's name
 *@return NULL if name = NULL else the file_name's path  
 */
char *get_file_path(char *file_name, char *name);
    
/**
 *@function strdup
 *@brief duplicate string adding the \0 character
 *@param s is the string
 *@return NULL if s = NULL else the duplicated character
 */
char *mystrdup(char *s);
#endif
