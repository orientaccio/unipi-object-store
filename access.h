#if !defined(ACCESS_H_)
#define ACCESS_H_
#define _POSIX_C_SOURCE 200112L //per strtok_r
#include <access.h>
#include <stdio.h>
#include <string.h>
#include <connection.h>
#include <unistd.h>
#include <stddef.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAXNAMELEN 32
#define BUFF_LEN 512


#define CHECK(r, c, e)              \
    if ((r = c) == 0) {              \
        fprintf(stderr, "%s\n", e);\
    }\
// al posto delle "guardie di inclusione" precedenti si puo' usare
// (anche se non standard)
// #pragma once
int os_connect(char *name);
int os_store(char *name, void *block, size_t len);
void *os_retrieve(char *name);
int os_delete(char *name);
int os_disconnect();

#endif /* LIBOBJSTORE_ACCESS_H_ */
