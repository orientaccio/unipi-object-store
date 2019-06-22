#if !defined(ACCESS_H_)
#define ACCESS_H_

#define _POSIX_C_SOURCE 200112L //per strtok_r
#include <string.h>
#include <stddef.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include <pthread.h>

#define MAXNAMELEN 32
#define BUFF_LEN 512

#define CHECKZERO(r, c, e) \
    if ((r = c) == 0) { perror(e); }

int os_connect(char *name);
int os_store(char *name, void *block, size_t len);
void *os_retrieve(char *name);
int os_delete(char *name);
int os_disconnect();

#endif /* ACCESS_H */
