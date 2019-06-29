#include "structure.h"

// mutex global variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
client_t *connected_client;

int n_client = 0;
int n_items = 0;
long total_size = 0;

int is_connected(char *name) 
{
    fprintf(stderr, "connected:  %s\n", name);
    client_t *curr = connected_client;

    while (curr != NULL) 
    {
        if (strcmp(name, curr->name) == 0) return 1;
        curr = curr->next;
    }

    return 0;
}

client_t *client_init(long fd) 
{
    client_t *client = (client_t *)malloc(sizeof(client));
    client->next = NULL;
    client->name = NULL;
    client->fd = fd;
    return client;
}

client_t *client_add(client_t *client, char *name) 
{
    int notused;
    MUTEXCALL(notused, pthread_mutex_lock(&mutex), "lock error");

    if (is_connected(name)) 
    {  
        // TODO reply
        fprintf(stderr, "client is already connected\n");
        MUTEXCALL(notused, pthread_mutex_unlock(&mutex), "unlock error");
        // TODO exit handler
        return NULL;
    }
    
    if (connected_client == NULL) 
    {
        connected_client = client;
        connected_client->name = (char *)malloc(sizeof(char) * strlen(name) + 1);
        strcpy(connected_client->name, name);
        //n_client++;
        MUTEXCALL(notused, pthread_mutex_unlock(&mutex), "unlock error");
        return connected_client;
    }

    // setting client name
    client->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
    strcpy(client->name, name);
    
    // add to the list
    client_t *curr = connected_client;
    while (curr->next != NULL) 
        curr = curr->next;
    curr->next = client;
    
    //n_client++;
    fprintf(stderr, "client %s added.\n", name);
    
    MUTEXCALL(notused, pthread_mutex_unlock(&mutex), "unlock error");
    return client;
}

void client_remove(client_t *client) 
{
    int notused;
    MUTEXCALL(notused, pthread_mutex_lock(&mutex), "lock error");
    
    client_t *curr = connected_client;
    client_t *prev = NULL;
    if (client == NULL || client->name == NULL || curr == NULL) 
    {
        MUTEXCALL(notused, pthread_mutex_unlock(&mutex), "unlock error");
        return;
    }
    
    fprintf(stderr, "{%s}\n", client->name);
    
    while (curr->next != NULL && client != curr) 
    {
        prev = curr;
        curr = curr->next;
    }
    
    if (prev == NULL) 
        connected_client = curr->next;
    else 
        prev->next = curr->next;
    
    //n_client--;
    fprintf(stderr, "client: n:{%d}, name: {%s} \n", n_client, curr->name);
    free(curr->name);
    free(curr); 
    
    MUTEXCALL(notused, pthread_mutex_unlock(&mutex), "unlock error");
}
