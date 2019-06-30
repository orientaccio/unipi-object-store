/**
 * @file access.h
 * @brief functions used by the client to access on the server
 */

#if !defined(ACCESS_H_)
#define ACCESS_H_
#include <stddef.h>
#include "connection.h"
#include "utils.h"

/**
 * @function os_connect
 * @brief enstablish the connection
 * @param name of the client
 * @return 1 if success, 0 if fail
 */
int os_connect(char *name);

/**
 * @function os_store
 * @brief stores an object called *name with data *block and length len
 * @param name of the object
 * @param block is the file information
 * @param len is the block's length
 * @return 1 if success, 0 if fail
 */
int os_store(char *name, void *block, size_t len);

/**
 * @function os_retrieve
 * @brief retrieves an object's data
 * @param name of the object to retrieve
 * @return data if success, NULL if fail
 */
void *os_retrieve(char *name);

/**
 * @function os_delete
 * @brief delete the object called *name
 * @param name of the object to delete
 * @return 1 if success, 0 if fail
 */
int os_delete(char *name);

/**
 * @function os_disconnect
 * @brief disconnect from the session
 * @return 1 if success, 0 if fail
 */
int os_disconnect();

#endif /* ACCESS_H */
