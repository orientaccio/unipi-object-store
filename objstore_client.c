#include <access.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Da levare
#include <math.h>

#define START_SIZE 100  // start offset
#define INC_SIZE 1000   // modify this to reach 100000 bytes

int totalOp = 0;
int successOp = 0;
int failedOp = 0;

void test1() {
    char data_name[3];
    char data_sing[5] = "bello";
    char* data = (char*)malloc(sizeof(char) * START_SIZE);
    int res, i = 0;

    for (i = 0; i < 20; i++) {
        totalOp++;
        long current_size = START_SIZE + i * INC_SIZE;
        data = realloc(data, (sizeof(char) * current_size));

        sprintf(data_name, "%d", i);
        while (current_size - strlen(data) > 0) {
            sprintf(data + strlen(data), "%s", data_sing);
        }

        CHECK(res, os_store(data_name, data, strlen(data)), "Error STORE");
        
        if (res == -1)
            failedOp++;
        else
            successOp++;
    }

    free(data);
}
/*
Recuperare oggetti verificando che i contenuti siano corretti**
 */
void test2() {
    char* data_name = "test2";
    char* data_store = "bello";
    char* data_retrieve;
    totalOp++;
    int res;
    CHECK(res, os_store(data_name, data_store, strlen(data_store)), "Error STORE");
    CHECK(data_retrieve, (char*)os_retrieve(data_name), "Error Retrieve");

    if (equal(data_retrieve, data_store)) {
        successOp++;
        fprintf(stderr, "Test2 OK\n");
        return;
    }
    free(data_retrieve);
    failedOp++;
    fprintf(stderr, "Test2 KO\n");

}

/*
**cancellare oggetti**
*/
void test3() {
    char* data_name = "test3";
    char* data_store = "bello";
    int res;
    totalOp++;
    CHECK(res, os_store(data_name, data_store, strlen(data_store)), "Error STORE");
    CHECK(res, os_delete(data_name), "Error DELETE");

    if (res != 1) {
        failedOp++;
        fprintf(stderr, "Test3 KO\n");
        return;
    }
    successOp++;
    fprintf(stderr, "Test3 OK\n");
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        fprintf(stderr, "usa: %s stringa \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && strlen(argv[1]) > MAXNAMELEN) {
        fprintf(stderr, "Name can be max %d char\n", MAXNAMELEN);
        exit(EXIT_FAILURE);
    }

    int res = 0;
    char* username = argv[1];
    fprintf(stderr, "CLIENT:%s\n", username);
    CHECK(res, os_connect(username), "Connection error");
    if (res == 0) return 0;

    int run;
    if (argc == 2)
        run = 1;
    else if (argc == 3) {  // caso in cui scelgo di usare i test
        switch (strtol(argv[2], NULL, 10)) {
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
        fprintf(stderr,
                "\n--------Risultati test--------\n\
                Op totali: %d\n\
                Op success: %d\n\
                Op fallite: %d\n\n\n",
                totalOp, successOp, failedOp);

        CHECK(res, os_disconnect(), "Error LEAVE");
        exit(EXIT_SUCCESS);
    }

    int scelta = 0;

    while (run) {
        printf(
            "\nSelect operation:\n\
                1)STORE\n\
                2)RETRIEVE\n\
                3)DELETE\n\
                4)LEAVE\n");
        scelta = 0;
        scanf("%d", &scelta);
        char dataName[33];
        char* data;
        switch (scelta) {
            case 1:
                printf("Insert data name:");
                scanf("%s", dataName);
                printf("Insert data:\n");
                scanf("%*c%ms", &data);  // ms alloca data dinamicamente
                CHECK(res, os_store(dataName, data, strlen(data)), "Error STORE");
                free(data);
                break;
            case 2:
                printf("Insert data name:");
                scanf("%s", dataName);
                CHECK(data, os_retrieve(dataName), "Error RETRIEVE");

                if (data != NULL) {
                    fprintf(stderr, "DATA: {%s}", data);
                    free(data);
                }
                break;
            case 3:
                printf("Insert data name:");
                scanf("%s", dataName);
                CHECK(res, os_delete(dataName), "Error DELETE");
                break;
            case 4:
                CHECK(res, os_disconnect(), "Error LEAVE");
                exit(EXIT_SUCCESS);
                break;
            default:
                perror("Error");
                break;
        }
        // sleep(3);

        fprintf(stderr, "FINE OP\n");
    }

    return 0;
}
