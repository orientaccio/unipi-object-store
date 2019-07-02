#include "structure.h"

// mutex global variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
client_t *connected_clients;

int n_client = 0;
int n_items = 0;
long total_size = 0;

int is_connected(char *name) 
{
    client_t *curr = connected_clients;
    while (curr != NULL) 
    {
        if (strcmp(name, curr->name) == 0) 
            return 1;
        curr = curr->next;
    }

    return 0;
}

client_t *client_init(long fd) 
{
    client_t *client;
    CHECKNULL(client, (client_t *) malloc(sizeof(client_t)), EMALLOC);
    client->next = NULL;
    client->name = NULL;
    client->fd = fd;
    return client;
}

client_t *client_add(client_t *client, char *name) 
{
    MUTEXCALL(pthread_mutex_lock(&mutex), "lock error");

    // check if already connected
    if (is_connected(name)) 
    {  
        // TODO reply
        // TODO exit handler
        MUTEXCALL(pthread_mutex_unlock(&mutex), "unlock error");
        return NULL;
    }
    
    // check if it is the first client
    if (connected_clients == NULL) 
    {
        connected_clients = client;
        CHECKNULL(connected_clients->name, (char *) malloc(sizeof(char) * strlen(name) + 1), EMALLOC);
        strcpy(connected_clients->name, name);
        n_client++;
        
        MUTEXCALL(pthread_mutex_unlock(&mutex), "unlock error");
        return connected_clients;
    }

    CHECKNULL(client->name, (char *) malloc(sizeof(char) * strlen(name) + 1), EMALLOC);
    strcpy(client->name, name);
    
    // add to the list
    client_t *curr = connected_clients;
    while (curr->next != NULL) 
        curr = curr->next;
    curr->next = client;
    
    n_client++;
    
    MUTEXCALL(pthread_mutex_unlock(&mutex), "unlock error");
    return client;
}

void client_remove(client_t *client) 
{
    MUTEXCALL(pthread_mutex_lock(&mutex), "lock error");
    
    client_t *curr = connected_clients;
    client_t *prev = NULL;
    if (client == NULL || client->name == NULL || curr == NULL) 
    {
        MUTEXCALL(pthread_mutex_unlock(&mutex), "unlock error");
        return;
    }
    
    while (curr->next != NULL && client != curr) 
    {
        prev = curr;
        curr = curr->next;
    }
    
    if (prev == NULL) 
        connected_clients = curr->next;
    else 
        prev->next = curr->next;
    
    n_client--;
    free(curr->name);
    free(curr); 
    
    MUTEXCALL(pthread_mutex_unlock(&mutex), "unlock error");
}
