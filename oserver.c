#include "threadF.h"

volatile sig_atomic_t received = 0;

int is_dot(const char dir[]);
void count_items(char *dir_name);
void print_status();
void signal_manager();
void signal_handler();

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
        if ((connfd = accept(listenfd, (struct sockaddr *)NULL, NULL)) == -1 && errno == EINTR && received == 0) 
            perror("accept");
        
        if (received == 1)
        {
            print_status();
            received = 0;
        }
        else
            spawn_thread(connfd);
    }
    unlink(SOCKNAME);
    return 0;
}

void signal_manager() 
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    // sa.sa_flags = ERESTART;

    int notused;
    SYSCALL(notused, sigaction(SIGUSR1, &sa, NULL), "sigaction");
}

void signal_handler() 
{ 
    received = 1; 
}

int is_dot(const char dir[]) 
{
    int l = strlen(dir);

    if ((l > 0 && dir[l - 1] == '.')) 
        return 1;
    return 0;
}

void count_items(char *dir_name) 
{    
    // open directory
    DIR *dir;
    CHECKNULL(dir, opendir(dir_name), "opendir");

    struct dirent *file;
    while ((file = readdir(dir)) != NULL) 
    {
        struct stat statbuf;
        char filename[512];
        strncpy(filename, dir_name, strlen(dir_name) + 1);
        strncat(filename, "/", 2);
        strncat(filename, file->d_name, strlen(file->d_name) + 1);

        if (is_dot(filename)) 
            continue;

        if (stat(filename, &statbuf) == -1) 
        {
            perror("stat error");
            return;
        }

        // recursive count if the file is a directory
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

void print_status()
{
    fprintf(stderr,
        "\nCURRENT SERVER STATUS ==============\n\
    Clients online: %d\n\
    Items stored: %d\n\
    Storage size: %ld\n\n",
        n_client, n_items, total_size);
}
