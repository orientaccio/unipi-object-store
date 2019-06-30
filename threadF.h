/**
 * @file threadF.h
 * @brief functions to manage threads
 */

#if !defined(THREAD_F_H_)
#define THREAD_F_H_

#include "structure.h"

/**
 * @function threadF
 * @brief execution code of the thread: manages the client requests
 * @param args is the argument passed to t he thread
 */
void *threadF(void *arg);

/**
 * @function spawn_thread
 * @brief creates a new threadF and detaches it
 * @param connfd is the connection file descriptor
 */
void spawn_thread(long connfd);

#endif
