#include "access.h"

// socket
static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFSIZE];
char *message_error;

// compares the first byte
int startcmp(char *str1, char *str2) 
{
    if (str1 == NULL || str2 == NULL) return 0;
    return (str1[0] == str2[0]);
}

int os_connect(char *name) 
{
    // socket creation
    SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), ESOCKET);
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    // socket connection
    int notused;
    SYSCALL(notused, connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), EREGISTER);
    if (notused != 0) 
        return 0;

    // message creation protocol
    msg_t request;
    char *command = "REGISTER";
    request.len = strlen(name) + strlen(command) + 3;
    CHECKNULL(request.str, (char*) calloc(request.len, sizeof(char)), EMALLOC);
    snprintf(request.str, request.len, "%s %s \n", command, name);
    
    // send message
    SYSCALL(notused, write(sockfd, request.str, request.len), EWRITE);
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), EREAD);
    free(request.str);
    
    fprintf(stderr, "%s\n", buffer);
    
    // response message check
    char *saveptr;
    char *commandr = strtok_r(buffer, " ", &saveptr);
    
    if (startcmp(commandr, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);
    if (startcmp(commandr, "OK")) 
        return 1;

    // error
    close(sockfd);
    return 0;
}

int os_store(char *name, void *block, size_t len) 
{
    // string to contain data
    long data_len = (long)len;
    int data_len_n = log10(data_len) + 1;
    char *data_len_s;
    CHECKNULL(data_len_s, (char*) malloc((data_len_n + 1) * sizeof(char)), EMALLOC);
    sprintf(data_len_s, "%ld", data_len);

    // message creation protocol
    msg_t request;
    char *command = "STORE";
    request.len = sizeof(char) * (strlen(command) + data_len + strlen(name) + strlen(data_len_s) + 5 + 1);  
    CHECKNULL(request.str, (char *) calloc(request.len, sizeof(char)), EMALLOC);
    snprintf(request.str, request.len, "%s %s %s \n %s", command, name, data_len_s, (char *)block);

    // send message
    int notused;
    SYSCALL(notused, writen(sockfd, request.str, request.len), EWRITE);
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), EREAD);
    
    free(request.str);
    free(data_len_s);
    
    // response message check
    char *saveptr;
    char *commandr = strtok_r(buffer, " ", &saveptr);
    
    if (startcmp(commandr, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);
    
    return (startcmp(commandr, "OK"));
}

void *os_retrieve(char *name) 
{
    // message creation protocol
    msg_t request;
    char *command = "RETRIEVE";
    request.len = (strlen(command) + strlen(name) + 3);
    CHECKNULL(request.str, (char*) calloc(request.len, sizeof(char)), EMALLOC);
    snprintf(request.str, request.len, "%s %s \n", command, name);

    // write & read message
    int notused;
    SYSCALL(notused, write(sockfd, request.str, request.len * sizeof(char)), EWRITE);
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), EREAD);
    free(request.str);
    
    // response message check
    char *saveptr;
    char *commandr = strtok_r(buffer, " ", &saveptr);
    
    if (strcmp(commandr, "DATA") == 0) 
    {
        char *data_len = strtok_r(NULL, " ", &saveptr);
        char *file_data = strtok_r(NULL, " \n", &saveptr);
        
        long first_read_len = strlen(file_data);
        long file_len = strtol(data_len, NULL, 10);
        long n_read = (long) ceilf((float)(file_len - first_read_len) / BUFSIZE);

        char *data;
        CHECKNULL(data, (char *) malloc(sizeof(char) * (file_len + 1)), EMALLOC);
        // file_len+1 o scoppia tutto
        int cx = snprintf(data, file_len + 1, "%s", file_data);

        while (n_read > 0) 
        {
            memset(buffer, '\0', BUFSIZE);
            
            SYSCALL(notused, read(sockfd, buffer, BUFSIZE), EREAD);
            cx += snprintf(data + cx, file_len - cx, "%s", file_data);
            n_read--;
        }

        return data;
    }
    
    if (startcmp(commandr, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);

    return NULL;
}

int os_delete(char *name) 
{
    // message creation protocol
    msg_t request;
    char *command = "DELETE";
    request.len = sizeof(char) * (strlen(command) + strlen(name) + 3);
    CHECKNULL(request.str, (char *) calloc(request.len, sizeof(char)), EMALLOC);
    snprintf(request.str, request.len, "%s %s \n", command, name);

    // write & read message
    int notused;
    SYSCALL(notused, write(sockfd, request.str, request.len * sizeof(char)), EWRITE);
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), EREAD);
    free(request.str);

    // response message check
    char *saveptr;
    char *commandr = strtok_r(buffer, " ", &saveptr);
    
    if (startcmp(commandr, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);
    
    return (startcmp(commandr, "OK"));
}

int os_disconnect() 
{
    // message creation protocol
    msg_t request;
    char *command = "LEAVE";
    request.len = strlen(command) + 3;
    CHECKNULL(request.str, (char *) calloc(request.len, sizeof(char)), EMALLOC);
    snprintf(request.str, request.len, "%s \n", command);

    // write & read message
    int notused;
    SYSCALL(notused, write(sockfd, request.str, strlen(request.str) * sizeof(char)), EWRITE);
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), EREAD);
    free(request.str);

    // response message check
    if (startcmp(buffer, "OK")) 
        close(sockfd);
    
    return (startcmp(buffer, "OK"));
}
