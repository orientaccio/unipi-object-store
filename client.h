#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include "connection.h"
#include "utils.h"

extern int n_client;
extern int n_items;
extern long total_size;

typedef struct client 
{
    char *name;
    struct client *next;
    long fd;
} client_t;

client_t *client_init(long fd);
client_t *client_add(client_t *client, char *name);
void client_remove(client_t *client);

#endif /* THREADPOOL_H_ */
