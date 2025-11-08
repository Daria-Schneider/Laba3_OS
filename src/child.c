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
    int has_filename;
    int has_command;
    int has_response;
    int shutdown;
} shared_data_t;

int main(void) {
    mmap_file_t mmap;
    FILE* file = NULL;
    int file_opened = 0;

    printf("Child process started\n");

    if (CpMmapOpen(&mmap, SHARED_MEM_NAME, sizeof(shared_data_t)) != 0) {
        printf("Error: failed to open shared memory\n");
        return EXIT_FAILURE;
    }

    shared_data_t* shared = (shared_data_t*)mmap.data;

    while (!shared->shutdown) {
        if (!file_opened && shared->has_filename) {
            char* filename = shared->filename;
            shared->has_filename = 0;
            CpMmapSync(&mmap);

            file = fopen(filename, "w");
            if (file == NULL) {
                snprintf(shared->response, sizeof(shared->response), 
                         "Error: cannot open file '%s' for writing", filename);
                shared->has_response = 1;
                shared->shutdown = 1;
                CpMmapSync(&mmap);
                break;
            }

            snprintf(shared->response, sizeof(shared->response), 
                     "File '%s' opened successfully", filename);
            shared->has_response = 1;
            file_opened = 1;
            CpMmapSync(&mmap);
            continue;
        }

        if (file_opened && shared->has_command) {
            char* command = shared->command;
            shared->has_command = 0;
            CpMmapSync(&mmap);

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
            
            shared->has_response = 1;
            CpMmapSync(&mmap);
        }
        
#ifdef _WIN32
        Sleep(10);
#else
        usleep(10000);
#endif
    }

    if (file) fclose(file);
    CpMmapClose(&mmap);
    
    printf("Child process finished\n");
    return EXIT_SUCCESS;
}