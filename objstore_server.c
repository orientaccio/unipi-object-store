#include "connection.h"
#include "utils.h"

// mutex global variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct client 
{
    char *name;
    struct client *next;
    long fd;
} client_t;

char *err_message;
client_t *connected_client;

int n_client = 0;
int n_items = 0;
long total_size = 0;

//void cleanup_thread_handler(void *arg) { close((long)arg); }
void cleanup() { unlink(SOCKNAME); }

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
        n_client++;
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
    
    n_client++;
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
    
    n_client--;
    fprintf(stderr, "client: n:{%d}, name: {%s} \n", n_client, curr->name);
    free(curr->name);
    free(curr); 
    
    MUTEXCALL(notused, pthread_mutex_unlock(&mutex), "unlock error");
}

char *get_dir_path(char *name) 
{
    int path_len = sizeof(char) * (strlen("data/") + strlen(name) + 1);
    char *path = (char *)malloc(path_len);
    snprintf(path, path_len, "data/%s", name);
    
    return path;
}

char *get_file_path(char *file_name, char *name) 
{
    char *dir = get_dir_path(name);
    int path_len = sizeof(char) * (strlen(dir) + strlen(name) + 2);
    char *path = (char *)malloc(path_len);
    snprintf(path, path_len, "%s/%s", dir, name);

    free(dir);
    return path;
}

client_t *manage_request(char *buf, client_t *client) 
{
    char *saveptr;
    char *comand = strtok_r(buf, " ", &saveptr);
    int result;
    
    if (client->name == NULL) 
    {
        if (strcmp(comand, "REGISTER") == 0) 
        {
            char *user = strtok_r(NULL, " ", &saveptr);
            client = client_add(client, user);
            if (client == NULL) 
            {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "register write error");
                return NULL;
            }
            
            char *path = get_dir_path(client->name);
            if (mkdir(path, 0777) == -1 && errno != EEXIST) 
            {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "directory creation error");
                free(path);
                return NULL;
            }
            free(path);
            SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");
            // TODO on error restore previous state
        } 
        else {  
            // TODO send reply incorrect request / not logged in //quit th?
            return NULL;
        }
    } 
    else 
    {
        if (strcmp(comand, "STORE") == 0) 
        {
            char *file_name = strtok_r(NULL, " ", &saveptr);
            char *file_len = strtok_r(NULL, " ", &saveptr);
            char *file_data = strtok_r(NULL, " \n", &saveptr);
            char *file_path = get_file_path(file_name, client->name);
            
            long file_length = strtol(file_len, NULL, 10);
            long first_read_len = strlen(file_data);
            FILE *fp1;

            CHECKNULL(fp1, fopen(file_path, "w"), EOPEN);
            if (fp1 == NULL) 
            {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "file creation error");

                free(file_path);
                return client;
            }

            long left_read_len = (long) ceil((double)(file_length - first_read_len) / BUFSIZE);
            fwrite(file_data, sizeof(char), first_read_len, fp1);

            while (left_read_len > 0) 
            {
                memset(buf, '\0', BUFSIZE);

                SYSCALL(result, read(client->fd, buf, BUFSIZE), "store read error");
                fprintf(stderr, "%s\n", buf);
                fwrite(buf, sizeof(char),
                       (left_read_len > 1) ? sizeof(char) * BUFSIZE : sizeof(char) * ((file_length - first_read_len) % BUFSIZE),
                       fp1);
                left_read_len--;
            }

            // add file length
            total_size += file_length;
            n_items++;

            fclose(fp1);
            free(file_path);
            SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "store write error");
        } 
        else if (strcmp(comand, "RETRIEVE") == 0) 
        {
            // create pathname
            char *file_name = strtok_r(NULL, " ", &saveptr);
            char *file_path = get_file_path(file_name, client->name);
            
            FILE *fpr;
            CHECKNULL(fpr, fopen(file_path, "r"), EOPEN);

            if (fpr == NULL) 
            {
                int error_len = strlen("KO \n") + strlen(err_message) + 2;
                char *message = (char *)malloc(error_len * sizeof(char));
                snprintf(message, error_len, "KO %s \n", err_message);
                SYSCALL(result, write(client->fd, message, error_len * sizeof(char)), "retrieve write error");
                
                free(file_path);
                return client;
            }

            // get file size
            struct stat st;
            stat(file_path, &st);
            long file_size = st.st_size;

            // read the file and prepare data message
            char *data = (char *)malloc(file_size * sizeof(char) + 1);
            char out;
            int counter = 0;
            
            while ((out = fgetc(fpr)) != EOF) 
                data[counter++] = (char)out;
            data[counter] = '\0';

            // prepare response message
            long data_len = strlen(data);
            int n_digits = log10(data_len) + 1;
            char *snum = (char *)malloc((n_digits + 1) * sizeof(char));
            sprintf(snum, "%ld", data_len);

            long response_len = strlen("DATA") + strlen(snum) + strlen(data) + 4 + 1;
            char *response = (char *)malloc(response_len * sizeof(char));
            snprintf(response, response_len, "DATA %s \n %s", snum, data);

            fprintf(stderr, "Reponse message: %s\n", response);

            SYSCALL(result, write(client->fd, response, response_len * sizeof(char)), "retrieve send error");

            free(snum);
            free(data);
            free(response);
            free(file_path);
            fclose(fpr);
        } 
        else if (strcmp(comand, "DELETE") == 0) 
        {
            // create pathname
            char *file_name = strtok_r(NULL, " ", &saveptr);
            char *file_path = get_file_path(file_name, client->name);

            // delete file
            if (remove(file_path) == 0) 
            {
                SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");
            }
            else
            {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "errore invio");
            }
            
            free(file_path);
        } 
        else if (strcmp(comand, "LEAVE") == 0) 
        {
            SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");
            fprintf(stderr, "%s leaves", client->name);
            client_remove(client);
            return NULL;
        } 
        else {
            // TODO send reply incorrect request / not logged in
            // quit thread
        }
    }
    return client;
}

void DEBUG_BUFFER(char *buffer, int result) 
{
    fprintf(stderr, "BUFFER:{");
    for (int i = 0; i < 512; i++) 
        fprintf(stderr, "%c", buffer[i]);
    fprintf(stderr, "} %d\n", result);
}

void *threadF(void *arg) 
{
    printf("New thread started\n");

    long connfd = (long)arg;
    client_t *client = client_init(connfd);
    char *buffer = (char *)malloc(sizeof(char) * BUFSIZE);
    int result = -1;

    do 
    {
        memset(buffer, '\0', BUFSIZE);
        SYSCALL(result, read(connfd, buffer, BUFSIZE), "errore lettura thread F");
        if (result < 1) 
            break;
        // DEBUG_BUFFER(buffer, result);
        client = manage_request(buffer, client);
        if (client == NULL) 
            break;

        fprintf(stderr, "	Thread F: %s %d \n", client->name, result);
    } 
    while (1);
    free(buffer);

    client_remove(client);
    close(connfd);
    pthread_exit("thread closed.\n");

    return NULL;
}

void printThrdError(int connfd, char *msg) 
{
    fprintf(stderr, "%s", msg);
    close(connfd);
}

void spawn_thread(long connfd) 
{
    pthread_attr_t thattr;
    pthread_t thid;
    /*
            sigset_t mask, oldmask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGINT);
            sigaddset(&mask, SIGQUIT);

            if (pthread_sigmask(SIG_BLOCK, &mask, &oldmask) != 0) {
                    printThrdError(connfd, "FATAL ERROR\n");
                    return;
            }
    */
    if (pthread_attr_init(&thattr) != 0) 
    {
        fprintf(stderr, "pthread_attr_init FALLITA\n");
        close(connfd);
        return;
    }
    // set the thread in detached mode
    if (pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED) != 0) 
    {
        fprintf(stderr, "pthread_attr_setdetachstate FALLITA\n");
        pthread_attr_destroy(&thattr);
        close(connfd);
        return;
    }

    if (pthread_create(&thid, &thattr, threadF, (void *)connfd) != 0) 
    {
        fprintf(stderr, "pthread_create FALLITA");
        pthread_attr_destroy(&thattr);
        close(connfd);
        return;
    }

    // if (pthread_sigmask(SIG_SETMASK, & oldmask, NULL) != 0)
    // printThrdError(connfd, "FATAL ERROR\n");
}

volatile sig_atomic_t received = 0;

void gestore() { received = 1; }

void signal_manager() 
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore;
    // sa.sa_flags = ERESTART;

    int notused;
    /*
     * change SIGTSTP to SIGUSR1
    
     * */
    SYSCALL(notused, sigaction(SIGUSR1, &sa, NULL), "sigaction");
}

int is_dot(const char dir[]) 
{
    int l = strlen(dir);

    if ((l > 0 && dir[l - 1] == '.')) 
        return 1;
    return 0;
}

void count_items(char *nomedir) 
{
    DIR *dir;
    if ((dir = opendir(nomedir)) == NULL) 
    {
        perror("opendir");
        return;
    }

    struct dirent *file;

    while ((file = readdir(dir)) != NULL) 
    {
        struct stat statbuf;
        char filename[512];
        strncpy(filename, nomedir, strlen(nomedir) + 1);
        strncat(filename, "/", 2);
        strncat(filename, file->d_name, strlen(file->d_name) + 1);

        if (is_dot(filename)) continue;

        if (stat(filename, &statbuf) == -1) 
        {
            perror("stat error");
            return;
        }

        // recursive print if file = directory
        if (S_ISDIR(statbuf.st_mode))
            count_items(filename);
        else 
        {
            n_items++;
            total_size += statbuf.st_size;
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) 
{
    cleanup();
    if (mkdir("data", 0777) == -1 && errno != EEXIST) exit(1);

    count_items("data");
    signal_manager();

    int listenfd = -1;
    SYSCALL(listenfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");

    struct sockaddr_un serv_addr;
    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;
    SYSCALL(notused, bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)), "bind");
    SYSCALL(notused, listen(listenfd, MAXBACKLOG), "listen");
    int connfd = -1;
    while (1) 
    {
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1 && errno == EINTR) 
        {
            perror("accept");
        }
        
        if (received == 0)
            spawn_thread(connfd);
        else 
        {
            fprintf(stderr,
                    "\n--------Ricevuto segnale--------\n\
                Clienti connessi: %d\n\
                Oggetti store: %d\n\
                Size totale store: %ld\n\n\n",
                    n_client + 1, n_items, total_size);
            received = 0;
        }
    }

    unlink(SOCKNAME);
    return 0;
}
