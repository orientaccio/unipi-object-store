#include "access.h"

static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFSIZE];
char *message_error;

int startcmp(char* str1, char* str2) 
{
    if (str1 == NULL || str2 == NULL) return 0;
    return (str1[0] == str2[0]);
}

int os_connect(char* name) 
{
    // socket creation
    SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    // socket connection
    int notused;
    SYSCALL(notused, connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect");
    if (notused != 0) 
        return 0;

    // message creation protocol
    char* type = "REGISTER";
    int message_len = sizeof(char) * (strlen(name) + strlen(type) + 3);  // 3 is for space and \n
    char* message = (char*) malloc(message_len);
    snprintf(message, message_len, "%s %s \n", type, name);
    
    // send message
    SYSCALL(notused, write(sockfd, message, message_len), "write");
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), "read");
    free(message);
    
    // response message check
    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    
    if (startcmp(command, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);
    if (startcmp(command, "OK")) 
        return 1;

    // error
    close(sockfd);
    return 0;
}

int os_store(char* name, void* block, size_t len) 
{
    // string to contain data
    long data_len = (long)len;
    int data_len_n = log10(data_len) + 1;
    char* data_len_s = (char*) malloc((data_len_n + 1) * sizeof(char));
    sprintf(data_len_s, "%ld", data_len);

    // message creation protocol
    char* message;
    char* type = "STORE";
    long message_len = sizeof(char) * (strlen(type) + data_len + strlen(name) + strlen(data_len_s) + 5 + 1);  
    message = (char*) malloc(message_len);
    snprintf(message, message_len, "%s %s %s \n %s", type, name, data_len_s, (char*)block);

    // send message
    int notused;
    SYSCALL(notused, write(sockfd, message, message_len), "write");
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), "read");
    
    free(message);
    free(data_len_s);
    
    // response message check
    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    
    if (startcmp(command, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);
    
    return (startcmp(command, "OK"));
}

void* os_retrieve(char* name) 
{
    // message creation protocol
    char* type = "RETRIEVE";
    long message_len = sizeof(char) * (strlen(type) + strlen(name) + 3);
    char* message = (char*) malloc((message_len) * sizeof(char));
    snprintf(message, message_len, "%s %s \n", type, name);

    // write & read message
    int notused;
    SYSCALL(notused, write(sockfd, message, message_len * sizeof(char)), "write");
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), "read");
    free(message);
    
    // response message check
    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    
    if (strcmp(command, "DATA") == 0) 
    {
        // Prendo le informazioni dall'Header
        char* data_len = strtok_r(NULL, " ", &saveptr);
        char* fileData = strtok_r(NULL, " \n", &saveptr);
        
        long lenFirstRead = strlen(fileData);
        long fileLength = strtol(data_len, NULL, 10);
        long nReadLeft = (long)ceil((double)(fileLength - lenFirstRead) / BUFSIZE);

        char* data = (char*) malloc(sizeof(char) * (fileLength + 1));
        // fileLength+1 o scoppia tutto
        int cx = snprintf(data, fileLength + 1, "%s", fileData);

        while (nReadLeft > 0) 
        {
            memset(buffer, '\0', BUFSIZE);
            
            SYSCALL(notused, read(sockfd, buffer, BUFSIZE), "errore lettura");
            cx += snprintf(data + cx, fileLength - cx, "%s", fileData);
            nReadLeft--;
        }

        return data;
    }
    
    if (startcmp(command, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);

    return NULL;
}

int os_delete(char* name) 
{
    // message creation protocol
    char* type = "DELETE";
    long message_len = sizeof(char) * (strlen(type) + strlen(name) + 3);
    char* message = (char*)malloc((message_len) * sizeof(char));
    snprintf(message, message_len, "%s %s \n", type, name);

    // write & read message
    int notused;
    SYSCALL(notused, write(sockfd, message, message_len * sizeof(char)), "write");
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), "read");
    free(message);

    // response message check
    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    
    if (startcmp(command, "KO")) 
        message_error = strtok_r(NULL, "\n", &saveptr);
    
    return (startcmp(command, "OK"));
}

int os_disconnect() 
{
    // message creation protocol
    char* type = "LEAVE";
    long message_len = sizeof(char) * (strlen(type) + 3);
    char* message = (char*)malloc((message_len) * sizeof(char));
    snprintf(message, message_len, "%s \n", type);

    // write & read message
    int notused;
    SYSCALL(notused, write(sockfd, message, strlen(message) * sizeof(char)), "write");
    SYSCALL(notused, read(sockfd, buffer, BUFSIZE * sizeof(char)), "read");
    free(message);

    // response message check
    if (startcmp(buffer, "OK")) 
        close(sockfd);
    
    return (startcmp(buffer, "OK"));
}
