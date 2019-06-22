#if !defined(CONNECTION_H)
#define CONNECTION_H

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>

#define SOCKNAME "./objstore.sock"
#define MAXBACKLOG 32
#define BUFFER_SIZE 512

#define SYSCALL(r, c, e) \
    if ((r = c) == -1) { perror(e); exit(errno); }                                  

#define CHECKNULL(r, c, e) \
    if ((r = c) == NULL) { perror(e); exit(errno); }

#define EOPEN "Error opening the file."
       
#endif /* CONNECTION_H */
