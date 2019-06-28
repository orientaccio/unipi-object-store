#include "utils.h"

char *get_dir_path(char *name) 
{
    int path_len = strlen("data/") + strlen(name) + 1;
    char *path = (char *) malloc(path_len * sizeof(char));
    snprintf(path, path_len, "data/%s", name);
    
    return path;
}

char *get_file_path(char *file_name, char *name) 
{
    char *dir = get_dir_path(name);
    int path_len = strlen(dir) + strlen(file_name) + 2;
    char *path = (char *) malloc(path_len * sizeof(char));
    snprintf(path, path_len, "%s/%s", dir, file_name);
    
    free(dir);
    return path;
}
