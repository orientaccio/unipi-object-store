#include "access.h"

#define START_SIZE 100  // start offset
#define INC_SIZE 1000   // modify this to reach 100000 bytes

int op_tot = 0;
int op_fail = 0;
int op_success = 0;

void test1();
void test2();
void test3();
void check_args(int argc, char *argv[]);
void debug_menu();

int main(int argc, char *argv[]) 
{
    check_args(argc, argv);
    debug_fprintf("%s\n\n", "TEST MODE ===============");
    
    // enstablish the connection
    int res;
    char *username = argv[1];
    CHECKZERO(res, os_connect(username), "connection error");
    if (res == 0) 
        exit(EXIT_FAILURE);

    // debug mode - test single requests
    if (DEBUG)
        while (1)
            debug_menu();
    
    switch (strtol(argv[2], NULL, 10)) 
    {
        case 1:
            test1();
            break;
        case 2:
            test2();
            break;
        case 3:
            test3();
            break;
        default:
            break;
    }
    
    // print results
    fprintf(stderr,
            "\nOPERATIONS RESULTS ===============\n\
            Operations success: %d\n\
            Operations failed: %d\n\
            Operations total: %d\n\n",
            op_success, op_fail, op_tot);
    
    CHECKZERO(res, os_disconnect(), "disconnect error");
    return 0;
}

void test1() 
{
    char data_name[3];
    char data_sing[5] = "bello";
    char *data;
    CHECKNULL(data, (char*)malloc(sizeof(char) * START_SIZE), "malloc");
    int res, i = 0;

    for (i = 0; i < 20; i++) 
    {
        op_tot++;
        long current_size = START_SIZE + i * INC_SIZE;
        data = realloc(data, (sizeof(char) * current_size));

        sprintf(data_name, "%d", i);
        while (current_size - strlen(data) > 0) {
            sprintf(data + strlen(data), "%s", data_sing);
        }

        CHECKZERO(res, os_store(data_name, data, strlen(data)), "Error STORE");
        fprintf(stderr, "RESPONSE: OK\n");
        
        op_success += res;
        op_fail += !res;
    }

    free(data);
}

void test2() 
{
    char *data_name = "test2";
    char *data_store = "bello";
    char *data_retrieve;
    int res;
    CHECKZERO(res, os_store(data_name, data_store, strlen(data_store)), "Error STORE");
    CHECKZERO(data_retrieve, (char*)os_retrieve(data_name), "Error Retrieve");
    op_tot++;
    
    if (strcmp(data_retrieve, data_store) == 0) 
    {
        op_success++;
        free(data_retrieve);
        fprintf(stderr, "Test2 OK\n");
        return;
    }
    
    op_fail++;
    free(data_retrieve);
    fprintf(stderr, "Test2 KO\n");
}

void test3() 
{
    char *data_name = "test3";
    char *data_store = "bello";
    int res;
    CHECKZERO(res, os_store(data_name, data_store, strlen(data_store)), "Error STORE");
    CHECKZERO(res, os_delete(data_name), "Error DELETE");
    op_tot++;
    
    if (res != 1) 
    {
        op_fail++;
        fprintf(stderr, "Test3 KO\n");
        return;
    }
    
    op_success++;
    fprintf(stderr, "Test3 OK\n");
}

void check_args(int argc, char *argv[]) 
{
    if (argc == 1) 
    {
        fprintf(stderr, "./objstore_client <name> <test>\n");
        exit(EXIT_FAILURE);
    }
    
    if (strlen(argv[1]) > MAXNAMELEN) 
    {
        fprintf(stderr, "max name_len %d char\n", MAXNAMELEN);
        exit(EXIT_FAILURE);
    }
    
    int n_test = (int)strtol(argv[2], NULL, 10);
    if (argc > 2 && (n_test > 3 || n_test < 1)) 
    {
        fprintf(stderr, "test number must be 1 <= n <= 3");
        exit(EXIT_FAILURE);
    }
}

void debug_menu() 
{
    printf(
            "\nSelect operation:\n\
                1)STORE\n\
                2)RETRIEVE\n\
                3)DELETE\n\
                4)LEAVE\n");
    char dataName[33];
    char *data;
    int scelta;
    int res;
    scanf("%d", &scelta);
    
    switch (scelta) 
    {
        case 1:
            printf("Insert data name:");
            scanf("%s", dataName);
            printf("Insert data:\n");
            scanf("%*c%ms", &data);  // ms alloca data dinamicamente
            CHECKZERO(res, os_store(dataName, data, strlen(data)), "Error STORE");
            free(data);
            break;
        case 2:
            printf("Insert data name:");
            scanf("%s", dataName);
            CHECKZERO(data, os_retrieve(dataName), "Error RETRIEVE");

            if (data != NULL) 
            {
                fprintf(stderr, "DATA: {%s}", data);
                free(data);
            }
            break;
        case 3:
            printf("Insert data name:");
            scanf("%s", dataName);
            CHECKZERO(res, os_delete(dataName), "Error DELETE");
            break;
        case 4:
            CHECKZERO(res, os_disconnect(), "Error LEAVE");
            exit(EXIT_SUCCESS);
            break;
        default:
            perror("Error");
            break;
    }
    
    fprintf(stderr, "FINE OP\n"); 
}
