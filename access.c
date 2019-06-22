#include <access.h>

static struct sockaddr_un serv_addr;
static int sockfd;

char buffer[BUFF_LEN];

/*
Check if str is equal to cmpstr
Returns 1 if true, 0 if false
*/
int equal(char* str, char* cmpstr) {
    if (str == NULL || cmpstr == NULL) return 0;
    return strcmp(str, cmpstr) != 0 ? 0 : 1;
}

/*
Check if str starts with cmpstr
Returns 1 if true, 0 if false
*/
int startsWith(char* str, char* cmpstr) {
    int slen;
    if (str == NULL || cmpstr == NULL) return 0;
    slen = strlen(cmpstr);
    if (strlen(str) < slen) return 0;
    for (int i = 0; i < slen; i++) {
        if (str[i] != cmpstr[i]) return 0;
    }
    return 1;
}

/*
Create a connection between server with SOCKNAME
Sets globally connfd and serv_addr
 */
int os_connect(char* name) {
    // Connessione socket
    SYSCALL(sockfd, socket(AF_UNIX, SOCK_STREAM, 0), "socket");

    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);

    int notused;

    SYSCALL(notused, connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)), "connect");
    if(notused!=0) return 0;


    // Creazione Header
    char* tipo = "REGISTER";
    int lenMex = sizeof(char) * (strlen(name) + strlen(tipo) + 3);  // 3 is for space and \n
    char* message = (char*)malloc(lenMex);

    snprintf(message, lenMex, "%s %s \n", tipo, name);
    fprintf(stderr, "Connecting: %s\n", name);

    // Invio request
    SYSCALL(notused, write(sockfd, message, lenMex), "write");
    free(message);

    SYSCALL(notused, read(sockfd, buffer, BUFF_LEN * sizeof(char)), "read");

    fprintf(stderr, "%s\n", buffer);

    if (startsWith(buffer, "OK")) return 1;
    close(sockfd);

    return 0;
}

int os_store(char* name, void* block, size_t len) {
    char* message;

    char* type = "STORE";

    // Creazione stringa che contiene la lunghezza del dato(size_t -> string)
    long lenData = (long)len;
    int numOfDigits = log10(lenData) + 1;                          // Numero di char che servono per scrivere lenData
    char* snum = (char*)malloc((numOfDigits + 1) * sizeof(char));  // Stringa per contenere lenData
    sprintf(snum, "%ld", lenData);
    // fprintf(stderr, "lunghezza  %s \n", snum);

    // Creazione header
    long lenMexToSend = sizeof(char) * (strlen(type) + lenData + strlen(name) + strlen(snum) + 5 +
                                        1);  // lunghezza messaggio (4 spazi + \n + terminazione)
    message = (char*)malloc(lenMexToSend);   // messaggio da inviare

    snprintf(message, lenMexToSend, "%s %s %s \n %s", type, name, snum, (char*)block);  // creo la stringa  da inviare

    free(snum);
    //fprintf(stderr, "Message to send: %s \n", message);

    // Invio request
    int notused;
    SYSCALL(notused, write(sockfd, message, lenMexToSend), "write");

    free(message);

    SYSCALL(notused, read(sockfd, buffer, BUFF_LEN * sizeof(char)), "read");

    fprintf(stderr, "RESPONSE: %s", buffer);

    if (startsWith((char*)buffer, "OK")) return 1;

    return 0;
}

void* os_retrieve(char* name) {
    // Creazione header
    char* type = "RETRIEVE";
    long lenMexToSend = sizeof(char) * (strlen(type) + strlen(name) + 3);
    char* message = (char*)malloc((lenMexToSend) * sizeof(char));

    snprintf(message, lenMexToSend, "%s %s \n", type, name);
    fprintf(stderr, "Message to send: %s \n", message);

    // Invio request
    int notused;
    SYSCALL(notused, write(sockfd, message, lenMexToSend * sizeof(char)), "write");
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFF_LEN * sizeof(char)), "read");
    // fprintf(stderr, "Buffer: %s\n\n", buffer);

    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    // Gestisco risposta di errore
    if (equal(command, "KO")) {
        char* errMsg = strtok_r(NULL, "\n", &saveptr);
        fprintf(stderr, "Messaggio di errore: %s\n", errMsg);
        return NULL;
    }
    // Creo il dato da ritornare
    else if (equal(command, "DATA")) {
        // Prendo le informazioni dall'Header
        char* lenData = strtok_r(NULL, " ", &saveptr);     // Stringa con la lunghezza del dato
        char* fileData = strtok_r(NULL, " \n", &saveptr);  // Prima parte del dato letta presente nel buffer
        long lenFirstRead = strlen(fileData);              // Lunghezza della prima parte letta

        long fileLength = strtol(lenData, NULL, 10);  // Trasformo la stringa della lunghezza in intero

        long nReadLeft = (long)ceil((double)(fileLength - lenFirstRead) /
                                    BUFFER_SIZE);  // Calcolo quante altre read dovrò fare per leggere l'intero dato
        fprintf(stderr, "nReadLeft: %ld\n", nReadLeft);

        char* data = (char*)malloc(sizeof(char) * (fileLength + 1));  // Alloco il dato da ritornare

        // fileLength+1 o scoppia tutto
        int cx = snprintf(data, fileLength + 1, "%s", fileData);  // uso cx per sapere il punto in cui sono arrivato a scrivere

        // Leggo la parte restante
        while (nReadLeft > 0) {
            memset(buffer, '\0', BUFFER_SIZE);  // Azzero il buffer

            int result;
            SYSCALL(result, read(sockfd, buffer, BUFFER_SIZE), "errore lettura");
            // fprintf(stderr, "%s\n", buffer);
            cx += snprintf(data + cx, fileLength - cx, "%s", fileData);  // Metto in append su data usando cx

            nReadLeft--;
        }

        fprintf(stderr, "cx: %d \n lenData: %s \n Data: %s\n", cx, lenData, data);

        return data;
    }

    fprintf(stderr, "Wrong command");
    return NULL;
}

int os_delete(char* name) {
    // Creo l'header
    char* type = "DELETE";
    long lenMexToSend = sizeof(char) * (strlen(type) + strlen(name) + 3);  // Calcolo la lunghezza messaggio
    char* message = (char*)malloc((lenMexToSend) * sizeof(char));

    snprintf(message, lenMexToSend, "%s %s \n", type, name);
    fprintf(stderr, "Message to send: %s \n", message);

    // invio request
    int notused;
    SYSCALL(notused, write(sockfd, message, lenMexToSend * sizeof(char)), "write");
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFF_LEN * sizeof(char)), "read");

    fprintf(stderr, "Buffer: %s", buffer);

    char* saveptr;
    char* command = strtok_r(buffer, " ", &saveptr);
    // Gestisco risposta di errore
    if (equal(command, "KO")) {
        char* errMsg = strtok_r(NULL, "\n", &saveptr);
        fprintf(stderr, "Messaggio di errore: %s", errMsg);
        return 0;
    } else if (equal(command, "OK"))
        return 1;

    return 0;
}

int os_disconnect() {
    // Creo l'header
    char* type = "LEAVE";
    long lenMexToSend = sizeof(char) * (strlen(type) + 3);
    char* message = (char*)malloc((lenMexToSend) * sizeof(char));

    snprintf(message, lenMexToSend, "%s \n", type);
    fprintf(stderr, "Message to send: %s \n", message);

    // invio request
    int notused;
    SYSCALL(notused, write(sockfd, message, strlen(message) * sizeof(char)), "write");
    free(message);

    // Aspetto la risposta
    SYSCALL(notused, read(sockfd, buffer, BUFF_LEN * sizeof(char)), "read");
    if (startsWith(buffer, "OK")) {
        close(sockfd);
        return 1;
    }
    close(sockfd);       // Chiudo il socket
    exit(EXIT_FAILURE);  // Da vedere se terminare il client oppure rendere possibile la connessione
    return 0;
}
