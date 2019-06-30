/**
 * @file structure.h
 * @brief client data structure and functions to manage the structure
 */

#ifndef STRUCTURE_H_
#define STRUCTURE_H_

#include "connection.h"
#include "utils.h"

/**
 * @var n_client numbers of the clients connected 
 * @var n_items number of the items in directory data/
 * @var total_size total size of the items in directory data/
 */
extern int n_client;
extern int n_items;
extern long total_size;

/**
 * @struct client_t
 * @brief client's informations
 * @var name is the client's name
 * @var next is the pointer to the next client
 * @var fd is the file descriptor
 */
typedef struct client 
{
    char *name;
    struct client *next;
    long fd;
} client_t;

/**
 * @function client_init
 * @brief initialize the client's informations
 * @param fd is the file descriptor
 * @return the client
 */
client_t *client_init(long fd);

/**
 * @function client_add
 * @brief add the client to the list of clients using mutex
 * @param client that needs to be added
 * @param name is the name of the client
 * @return the client
 */
client_t *client_add(client_t *client, char *name);

/**
 * @function client_remove
 * @brief remove the client from the list of clients using mutex
 * @param client that needs to be added
 */
void client_remove(client_t *client);

#endif
