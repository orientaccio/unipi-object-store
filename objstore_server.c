#define _POSIX_C_SOURCE 200112L
#include "connection.h"

// mutex global variable
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct client {
    char *name;
    struct client *next;
    long fd;
} t_client;

t_client *connectedClient;

int n_client = 0;
int n_items = 0;
long total_size = 0;

static char *myErrno;

void cleanup() { unlink(SOCKNAME); }

int equal(char *str, char *cmpstr) {
    if (str == NULL || cmpstr == NULL) return 0;
    return strcmp(str, cmpstr) != 0 ? 0 : 1;
}

int is_connected(char *name) {
    fprintf(stderr, "is_connected:  %s\n", name);
    t_client *curr = connectedClient;

    while (curr != NULL) {
        if (equal(name, curr->name)) return 1;
        curr = curr->next;
    }

    return 0;
}

static void cleanup_thread_handler(void *arg) { close((long)arg); }

t_client *initClient(long fd) {
    t_client *client = (t_client *)malloc(sizeof(t_client));
    client->next = NULL;
    client->name = NULL;
    client->fd = fd;
    // n_client++;
    return client;
}

t_client *addClient(t_client *client, char *name) {
    fprintf(stderr, "%s ", name);
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK

    if (connectedClient == NULL) {
        connectedClient = client;
        /*
        connectedClient = (t_client *)malloc(sizeof(t_client));
        connectedClient->next = NULL;*/
        connectedClient->name = (char *)malloc(sizeof(char) * strlen(name) + 1);
        strcpy(connectedClient->name, name);
        n_client++;
        pthread_mutex_unlock(&mutex);
        return connectedClient;
    }

    if (is_connected(name)) {  // TODO reply
        fprintf(stderr, "già connesso");
        pthread_mutex_unlock(&mutex);
        // TODO exit handler
        return NULL;
    }

    t_client *curr = connectedClient;

    while (curr->next != NULL) curr = curr->next;
    /*
    t_client *new = (t_client *)malloc(sizeof(t_client));*/
    client->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
    // new->next = NULL;

    strcpy(client->name, name);

    curr->next = client;

    n_client++;
    pthread_mutex_unlock(&mutex);  // Rilascio della LOCK
    return client;
}

void removeClient(t_client *client) {
    pthread_mutex_lock(&mutex);  // Acquisizione della LOCK
    t_client *curr = connectedClient;
    t_client *prev = NULL;
    if (client == NULL || client->name == NULL || curr == NULL /*|| n_client == 0*/) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    fprintf(stderr, "{%s}\n", client->name);
    while (curr->next != NULL && client != curr) {
        prev = curr;
        curr = curr->next;
    }
    if (prev == NULL) {  // se è il primo della lista
        connectedClient = curr->next;
    } else {
        prev->next = curr->next;
    }
    fprintf(stderr, "   rmv client: n:{%d}, name: {%s} \n", n_client, curr->name);
    n_client--;
    pthread_mutex_unlock(&mutex);
    free(curr->name);
    free(curr);  // Rilascio della LOCK
}

char *getDirPath(char *username) {
    int lenPath = sizeof(char) * (strlen("data/") + strlen(username) + 1);
    char *path = (char *)malloc(lenPath);
    snprintf(path, lenPath, "data/%s", username);
    return path;
}

char *getFilePath(char *fileName, char *username) {
    char *dir = getDirPath(username);
    int lenPath = sizeof(char) * (strlen(dir) + strlen(fileName) + 2);
    char *path = (char *)malloc(lenPath);

    snprintf(path, lenPath, "%s/%s", dir, fileName);

    free(dir);
    return path;
}

t_client *manageRequest(char *buf, t_client *client) {
    char *saveptr;
    char *comand = strtok_r(buf, " ", &saveptr);

    if (client->name == NULL) {
        if (equal(comand, "REGISTER")) {
            char *user = strtok_r(NULL, " ", &saveptr);
            client = addClient(client, user);
            int result;
            if (client == NULL) {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "errore invio");
                return NULL;
            }
            fprintf(stderr, "	INIT MANAGE REQ 2:  (%s)\n", client->name);
            char *dirPath = getDirPath(client->name);
            if (mkdir(dirPath, 0777) == -1 && errno != EEXIST) {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "errore nome path troppo grande");
                free(dirPath);
                return NULL;
            }
            free(dirPath);
            SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");

            // TODO on error restore previous state
        } else {  // TODO send reply incorrect request / not logged in //quit th?
            return NULL;
        }
    } else {
        // se è registrato
        if (equal(comand, "STORE")) {  // Se il file esiste lo sovrascrive (potemo fa come ce pare)
            // str = strtok(NULL, " ");   // nomefile
            char *fileName = strtok_r(NULL, " ", &saveptr);
            char *fileLen = strtok_r(NULL, " ", &saveptr);
            // fprintf(stderr, "%s", fileName);
            char *fileData = strtok_r(NULL, " \n", &saveptr);  // parte di <data> (strtok di \n per dati con spazi)
            char *fileToWrite = getFilePath(fileName, client->name);
            long fileLength = strtol(fileLen, NULL, 10);
            long lenFirstRead = strlen(fileData);
            int result;
            FILE *fp1;

            CHECKNULL(fp1, fopen(fileToWrite, "w"), EOPEN);
            if (fp1 == NULL) {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "errore nome file troppo grande");

                return client;
            }

            long nReadLeft = (long)ceil((double)(fileLength - lenFirstRead) / BUFFER_SIZE);
            int res = fwrite(fileData, sizeof(char), lenFirstRead, fp1);
            fprintf(stderr, "RES:%d\n", res);

            while (nReadLeft > 0) {
                memset(buf, '\0', BUFFER_SIZE);

                SYSCALL(result, read(client->fd, buf, BUFFER_SIZE), "errore lettura");
                fprintf(stderr, "%s\n", buf);
                fwrite(buf, sizeof(char),
                       (nReadLeft > 1) ? sizeof(char) * BUFFER_SIZE : sizeof(char) * ((fileLength - lenFirstRead) % BUFFER_SIZE),
                       fp1);
                nReadLeft--;
            }

            // add file length
            total_size += fileLength;
            n_items++;

            fclose(fp1);
            free(fileToWrite);
            SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");
        } else if (equal(comand, "RETRIEVE")) {
            // create pathname
            char *name = strtok_r(NULL, " ", &saveptr);
            int path_len = strlen("data") + strlen(client->name) + strlen(name) + 2 + 1;
            char *pathname = (char *)malloc(path_len * sizeof(char));
            snprintf(pathname, path_len, "%s/%s/%s", "data", client->name, name);

            fprintf(stderr, "Pathname: %s", pathname);

            // open the file
            FILE *fpr;
            CHECKNULL(fpr, fopen(pathname, "r"), EOPEN);

            // file error
            int result = 0;
            if (fpr == NULL) {
                free(pathname);
                int eMsgLen = strlen("KO \n") + strlen(myErrno) + 2;
                char *message = (char *)malloc(eMsgLen * sizeof(char));
                snprintf(message, eMsgLen, "KO %s \n", myErrno);
                SYSCALL(result, write(client->fd, message, eMsgLen * sizeof(char)), "errore invio");
                return client;
            }

            // get file size
            struct stat st;
            stat(pathname, &st);
            long file_size = st.st_size;

            // read the file and prepare data message
            char *data = (char *)malloc(file_size * sizeof(char) + 1);
            char out;
            int counter = 0;
            while ((out = fgetc(fpr)) != EOF) data[counter++] = (char)out;
            data[counter] = '\0';

            // prepare response message
            long data_len = strlen(data);

            int numOfDigits = log10(data_len) + 1;                          // Numero di char che servono per scrivere lenData
            char *snum = (char *)malloc((numOfDigits + 1) * sizeof(char));  // stringa per contenere lenData
            sprintf(snum, "%ld", data_len);

            long response_len = strlen("DATA") + strlen(snum) + strlen(data) + 4 + 1;
            char *response = (char *)malloc(response_len * sizeof(char));
            snprintf(response, response_len, "DATA %s \n %s", snum, data);

            fprintf(stderr, "\nReponse message: %s", response);

            SYSCALL(result, write(client->fd, response, response_len * sizeof(char)), "errore invio");

            free(snum);
            free(data);
            free(response);
            free(pathname);
            fclose(fpr);

        } else if (equal(comand, "DELETE")) {
            // create pathname
            char *name = strtok_r(NULL, " ", &saveptr);
            int path_len = strlen("data") + strlen(client->name) + strlen(name) + 2 + 1;
            char *pathname = (char *)malloc(path_len * sizeof(char));
            snprintf((char *)pathname, path_len, "%s/%s/%s", "data", client->name, name);

            fprintf(stderr, "Pathname: %s", pathname);

            // delete file
            int result;
            if (remove(pathname) == 0) {
                SYSCALL(result, write(client->fd, "OK \n", 5 * sizeof(char)), "errore invio");
            } else {
                SYSCALL(result, write(client->fd, "KO \n", 5 * sizeof(char)), "errore invio");
            }

            free(pathname);
        } else if (equal(comand, "LEAVE")) {
            fprintf(stderr, "uscito");
            removeClient(client);
            return NULL;
        } else {
            // TODO send reply incorrect request / not logged in
            // quit thread
        }
    }
    return client;
}

void DEBUG_BUFFER(char *buffer, int result) {
    fprintf(stderr, "   BUFFER:{");
    for (int i = 0; i < 512; i++) fprintf(stderr, "%c", buffer[i]);
    fprintf(stderr, "}FINE BUFFER %d\n", result);
}
void *threadF(void *arg) {
    printf("New thread started\n");

    long connfd = (long)arg;
    t_client *client = initClient(connfd);
    char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
    int result = -1;

    do {
        memset(buffer, '\0', BUFFER_SIZE);
        SYSCALL(result, read(connfd, buffer, BUFFER_SIZE), "errore lettura thread F");
        if (result < 1) break;
        // DEBUG_BUFFER(buffer, result);
        client = manageRequest(buffer, client);
        if (client == NULL) break;

        fprintf(stderr, "	Thread F: %s %d \n", client->name, result);
    } while (1);
    free(buffer);

    fprintf(stderr, "	Client: ");
    removeClient(client);
    fprintf(stderr, " terminato\n");
    close(connfd);
    pthread_exit("Thread closed");

    return NULL;
}

void printThrdError(int connfd, char *msg) {
    fprintf(stderr, "%s", msg);
    close(connfd);
}

void spawn_thread(long connfd) {
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
    if (pthread_attr_init(&thattr) != 0) {
        fprintf(stderr, "pthread_attr_init FALLITA\n");
        close(connfd);
        return;
    }
    // settiamo il thread in modalità detached
    if (pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED) != 0) {
        fprintf(stderr, "pthread_attr_setdetachstate FALLITA\n");
        pthread_attr_destroy(&thattr);
        close(connfd);
        return;
    }

    if (pthread_create(&thid, &thattr, threadF, (void *)connfd) != 0) {
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

void signal_manager() {
    struct sigaction sa;
    // resetto la struttura
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = gestore;
    // sa.sa_flags = ERESTART;

    int notused;
    /*
     *
     * change SIGTSTP to SIGUSR1
     *
     * */
    SYSCALL(notused, sigaction(SIGUSR1, &sa, NULL), "sigaction");
}

int is_dot(const char dir[]) {
    int l = strlen(dir);

    if ((l > 0 && dir[l - 1] == '.')) return 1;
    return 0;
}

void count_items(char *nomedir) {
    DIR *dir;
    if ((dir = opendir(nomedir)) == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *file;

    while ((file = readdir(dir)) != NULL) {
        struct stat statbuf;
        char filename[512];
        strncpy(filename, nomedir, strlen(nomedir) + 1);
        strncat(filename, "/", 2);
        strncat(filename, file->d_name, strlen(file->d_name) + 1);

        if (is_dot(filename)) continue;

        if (stat(filename, &statbuf) == -1) {
            perror("eseguendo la stat");
            return;
        }

        // recursive print if file = directory
        if (S_ISDIR(statbuf.st_mode))
            count_items(filename);
        else {
            n_items++;
            total_size += statbuf.st_size;
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
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
    while (1) {
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1 && errno == EINTR) {
            perror("accept");
        }
        if (received == 0)
            spawn_thread(connfd);
        else {
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
