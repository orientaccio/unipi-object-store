#if !defined(THREAD_F_H_)
#define THREAD_F_H_

#include "structure.h"

void *threadF(void *arg);
void spawn_thread(long connfd);

#endif
