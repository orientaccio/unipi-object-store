#include "utils.h"

char *get_dir_path(char *name) 
{
    if (name == NULL)
        return NULL;
        
    int path_len = strlen("data/") + strlen(name) + 1;
    char *path;
    CHECKNULL(path, (char *) malloc(path_len * sizeof(char)), "malloc");
    snprintf(path, path_len, "data/%s", name);
    
    return path;
}

char *get_file_path(char *file_name, char *name) 
{
    if (file_name == NULL || name == NULL)
        return NULL;
    
    char *dir = get_dir_path(name);
    int path_len = strlen(dir) + strlen(file_name) + 2;
    char *path;
    CHECKNULL(path, (char *) malloc(path_len * sizeof(char)), "malloc");
    snprintf(path, path_len, "%s/%s", dir, file_name);
    free(dir);
    
    return path;
}
