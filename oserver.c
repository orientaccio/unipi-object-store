#include "threadF.h"

volatile sig_atomic_t received = 0;

void gestore() 
{ 
    received = 1; 
}

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

        if (is_dot(filename)) 
            continue;

        if (stat(filename, &statbuf) == -1) 
        {
            perror("stat error");
            return;
        }

        // recursive print if file is a directory
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
    unlink(SOCKNAME);
    if (mkdir("data", 0777) == -1 && errno != EEXIST) 
        exit(1);

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
