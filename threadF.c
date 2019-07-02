#include "threadF.h"

char *err_message;

void send_message(client_t *client, char *header, char *message)
{
    if (client == NULL || header == NULL)
        return;
    
    msg_t response;
    int result;
    
    // calculate message length
    response.len = strlen(header) + 3;
    if (message != NULL)
         response.len += strlen(message) + 1;
    
    // create response message
    CHECKNULL(response.str, (char *) calloc(response.len, sizeof(char)), EMALLOC);
    if (strcmp(header, "OK") == 0)
        snprintf(response.str, response.len, "%s \n", header);
    else
        snprintf(response.str, response.len, "%s %s \n", header, message);
    
    fprintf(stderr, "Response message: %s", response.str);
    
    // writes
    SYSCALL(result, write(client->fd, response.str, response.len * sizeof(char)), EWRITE);
    free(response.str);
}

client_t *manage_request(char *buf, client_t *client) 
{
    char *saveptr;
    char *comand = strtok_r(buf, " ", &saveptr);
    int result;
    
    //if (client->name != NULL)
        //fprintf(stderr, "%s %s\n", client->name, comand);
    
    if (client->name == NULL && strcmp(comand, "REGISTER") != 0) 
    {
        send_message(client, "KO", EINVALID);
        return NULL;
    }
    
    if (strcmp(comand, "REGISTER") == 0) 
    {
        char *name = strtok_r(NULL, " ", &saveptr);
        client = client_add(client, name);
        if (client == NULL) 
        {
            send_message(client, "KO", EREGISTER);
            return NULL;
        }
        
        char *path = get_dir_path(client->name);
        if (mkdir(path, 0777) == -1 && errno != EEXIST) 
        {
            send_message(client, "KO", EREGISTER);
            free(path);
            return NULL;
        }
    
        fprintf(stderr, "%s %s\n", client->name, comand);
    
        send_message(client, "OK", NULL);
        free(path);
    } 
    else if (strcmp(comand, "STORE") == 0) 
    {
        char *file_name = strtok_r(NULL, " ", &saveptr);
        char *file_len = strtok_r(NULL, " ", &saveptr);
        char *file_data1 = strtok_r(NULL, " \n", &saveptr);
        char *file_data2 = mystrdup(file_data1);
        char *file_path = get_file_path(file_name, client->name);
        
        long file_length = strtol(file_len, NULL, 10);
        long first_read_len = strlen(file_data2);
        FILE *fp1;
        
        if ((fp1 = fopen(file_path, "w")) == NULL) 
        {
            perror(EOPEN);
            send_message(client, "KO", ESTORE);
            free(file_path);
            return client;
        }

        long n_read = (long) ceilf((file_length - first_read_len) / BUFSIZE);
        fwrite(file_data2, sizeof(char), first_read_len, fp1);
        
        while (n_read > 0) 
        {
            memset(buf, '\0', BUFSIZE);
            SYSCALL(result, read(client->fd, buf, BUFSIZE), EREAD);
            int read_len = (n_read > 1) ? BUFSIZE : ((file_length - first_read_len) % BUFSIZE);
            fwrite(buf, sizeof(char), read_len, fp1);
            n_read--;
        }

        send_message(client, "OK", ESTORE);
        free(file_data2);
        free(file_path);
        fclose(fp1);
    } 
    else if (strcmp(comand, "RETRIEVE") == 0) 
    {
        // create pathname
        char *file_name = strtok_r(NULL, " ", &saveptr);
        char *file_path = get_file_path(file_name, client->name);
        
        FILE *fpr;
        if ((fpr = fopen(file_path, "r")) == NULL) 
        {
            send_message(client, "KO", ERETRIEVE);
            free(file_path);
            return client;
        }

        // get file size
        struct stat st;
        stat(file_path, &st);
        long file_size = st.st_size;

        // read the file and prepare data message
        int counter = 0;
        char out;
        char *data;
        CHECKNULL(data, (char *) malloc(file_size * sizeof(char) + 1), EMALLOC);
        
        while ((out = fgetc(fpr)) != EOF) 
            data[counter++] = (char)out;
        data[counter] = '\0';

        // prepare response message
        long data_len = strlen(data);
        int n_digits = log10(data_len) + 1;
        char *snum;
        CHECKNULL(snum, (char *)malloc((n_digits + 1) * sizeof(char)), EMALLOC);
        sprintf(snum, "%ld", data_len);

        long response_len = strlen("DATA") + strlen(snum) + strlen(data) + 4 + 1;
        char *response;
        CHECKNULL(response, (char *)malloc(response_len * sizeof(char)), EMALLOC);
        snprintf(response, response_len, "DATA %s \n %s", snum, data);

        fprintf(stderr, "Reponse message: %s\n", "DATA");

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
            send_message(client, "OK", NULL);
        else
            send_message(client, "KO", EDELETE);
        
        free(file_path);
    } 
    else if (strcmp(comand, "LEAVE") == 0) 
    {
        send_message(client, "OK", NULL);
        client_remove(client);
        return NULL;
    } 
//     else 
//         send_message(client, "KO", EINVALID);
    
    return client;
}

void *threadF(void *arg) 
{
    long connfd = (long)arg;
    char *buffer;
    client_t *client = client_init(connfd);
    CHECKNULL(buffer, (char *) calloc(BUFSIZE + 1, sizeof(char)), EMALLOC);
    int result = -1;

    do 
    {
        memset(buffer, '\0', BUFSIZE);
        SYSCALL(result, read(connfd, buffer, BUFSIZE), "errore lettura thread F");
        if (result < 1) 
            break;
        
        client = manage_request(buffer, client);
        if (client == NULL) 
            break;
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
