#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crossplatform.h"
#include "stringutils.h"

#define BUFFER_SIZE 1024

typedef struct {
    char filename[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int ready;
    int shutdown;
} shared_data_t;

int main(void) {
    mmap_file_t mmap;
    FILE* file = NULL;

    printf("Child process started\n");

    if (CpMmapOpen(&mmap, SHARED_MEM_NAME, sizeof(shared_data_t)) != 0) {
        printf("Error: failed to open shared memory\n");
        return EXIT_FAILURE;
    }

    shared_data_t* shared = (shared_data_t*)mmap.data;

    while (shared->filename[0] == '\0' && !shared->shutdown) {
        CpMmapSync(&mmap);
    }

    if (shared->shutdown) {
        CpMmapClose(&mmap);
        return EXIT_FAILURE;
    }

    file = fopen(shared->filename, "w");
    if (file == NULL) {
        printf("Error: cannot open file '%s' for writing\n", shared->filename);
        shared->shutdown = 1;
        CpMmapSync(&mmap);
        CpMmapClose(&mmap);
        return EXIT_FAILURE;
    }

    printf("File '%s' opened successfully\n", shared->filename);
    shared->ready = 1;
    CpMmapSync(&mmap);

    while (!shared->shutdown) {
        if (shared->command[0] != '\0') {
            char* command = shared->command;
            
            if (strlen(command) == 0) {
                shared->shutdown = 1;
                CpMmapSync(&mmap);
                break;
            }

            if (IsCapitalStart(command)) {
                fprintf(file, "%s\n", command);
                fflush(file);
                snprintf(shared->response, sizeof(shared->response), "OK");
            } else {
                snprintf(shared->response, sizeof(shared->response), 
                         "ERROR: must start with capital letter");
            }
            
            shared->command[0] = '\0';
            CpMmapSync(&mmap);
        }
        CpMmapSync(&mmap);
    }

    if (file) fclose(file);
    CpMmapClose(&mmap);
    
    printf("Child process finished\n");
    return EXIT_SUCCESS;
}
