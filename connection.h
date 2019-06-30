#if !defined(CONNECTION_H)
#define CONNECTION_H

#define _POSIX_C_SOURCE 200112L
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
#define MAXNAMELEN 32
#define BUFSIZE 512
    
#define EOPEN "Error opening the file."
       
#endif
