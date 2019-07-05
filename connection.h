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
#define MAXNAMELEN 255
#define BUFSIZE 512

#define EINVALID    "Error invalid operation" 
#define EREGISTER   "Error registration"
#define ESTORE      "Error store"
#define ERETRIEVE   "Error retrieve"
#define EDELETE     "Error delete"

typedef struct msg {
    long len;
    char *str;
} msg_t;

static inline int writen(long fd, void *buf, size_t size) 
{
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    
    while (left>0) 
	{
		if ((r = write((int)fd ,bufptr,left)) == -1) 
		{
			if (errno == EINTR) 
				continue;
			return -1;
		}
		if (r == 0) 
			return 0;  
		left   -= r;
		bufptr += r;
    }
    return 1;
}

static inline int readn(long fd, void *buf, size_t size) 
{
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    
    while (left>0)
    {
        if ((r = read((int)fd ,bufptr,left)) == -1) 
        {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) 
            return 0;   // gestione chiusura socket
        left   -= r;
        bufptr += r;
    }
    return size;
}

#endif
