#include "threadF.h"

char *err_message;

void DEBUG_BUFFER(char *buffer, int result) 
{
    fprintf(stderr, "BUFFER:{");
    for (int i = 0; i < 512; i++) 
        fprintf(stderr, "%c", buffer[i]);
    fprintf(stderr, "} %d\n", result);
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

            fprintf(stderr, "file_name: %s\n", file_name);
            fprintf(stderr, "file_path: %s\n", file_path);
            
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

void *threadF(void *arg) 
{
    long connfd = (long)arg;
    client_t *client = client_init(connfd);
    char *buffer = (char *) malloc(sizeof(char) * BUFSIZE);
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

        fprintf(stderr, "Thread F: %s %d \n", client->name, result);
    } 
    while (1);
    free(buffer);

    client_remove(client);
    close(connfd);
    pthread_exit("thread closed.\n");

    return NULL;
}

void print_thread_error(int connfd, char *msg) 
{
    fprintf(stderr, "%s", msg);
    close(connfd);
}

void spawn_thread(long connfd) 
{
    pthread_attr_t thattr;
    pthread_t thid;
    
    //sigset_t mask, oldmask;
    //sigemptyset(&mask);
    //sigaddset(&mask, SIGINT);
    //sigaddset(&mask, SIGQUIT);

    //if (pthread_sigmask(SIG_BLOCK, &mask, &oldmask) != 0) {
    //        print_thread_error(connfd, "FATAL ERROR\n");
    //        return;
    //}
    
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
    // print_thread_error(connfd, "FATAL ERROR\n");
}
