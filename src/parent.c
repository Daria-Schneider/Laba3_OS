#include <stdlib.h>
#include <stdio.h>
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
    process_t child;
    mmap_file_t mmap;
    char line[BUFFER_SIZE];
    
    memset(&child, 0, sizeof(child));
    memset(&mmap, 0, sizeof(mmap));

    printf("Lab 3 - Parent process started\n");

    if (CpMmapCreate(&mmap, SHARED_MEM_NAME, sizeof(shared_data_t)) != 0) {
        fprintf(stderr, "Error: failed to create shared memory\n");
        return EXIT_FAILURE;
    }

    shared_data_t* shared = (shared_data_t*)mmap.data;
    memset(shared, 0, sizeof(shared_data_t));

    const char *childPath = CpGetChildProcessName("child");

    printf("Creating child process...\n");
    if (CpProcessCreate(&child, childPath) != 0) {
        fprintf(stderr, "Error: failed to create child process\n");
        CpMmapClose(&mmap);
        return EXIT_FAILURE;
    }

    printf("Enter file name: ");
    if (!fgets(line, sizeof(line), stdin)) {
        fprintf(stderr, "Error: failed to read file name\n");
        shared->shutdown = 1;
        CpMmapSync(&mmap);
        CpProcessClose(&child);
        CpMmapClose(&mmap);
        return EXIT_FAILURE;
    }
    TrimNewline(line);

    strncpy(shared->filename, line, sizeof(shared->filename) - 1);
    shared->filename[sizeof(shared->filename) - 1] = '\0';
    CpMmapSync(&mmap);

    while (!shared->ready && !shared->shutdown) {
        CpMmapSync(&mmap);
    }

    if (shared->shutdown) {
        printf("Child process failed to open file\n");
        CpProcessClose(&child);
        CpMmapClose(&mmap);
        return EXIT_FAILURE;
    }

    printf("Ready. Enter strings (empty to exit):\n");

    while (1) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        TrimNewline(line);

        if (strlen(line) == 0) {
            shared->shutdown = 1;
            CpMmapSync(&mmap);
            break;
        }

        strncpy(shared->command, line, sizeof(shared->command) - 1);
        shared->command[sizeof(shared->command) - 1] = '\0';
        CpMmapSync(&mmap);

        while (shared->response[0] == '\0' && !shared->shutdown) {
            CpMmapSync(&mmap);
        }

        if (shared->shutdown) break;

        printf("Result: %s\n", shared->response);
        shared->response[0] = '\0';
        CpMmapSync(&mmap);
    }

    CpProcessClose(&child);
    CpMmapClose(&mmap);
    
#ifndef _WIN32
    shm_unlink(SHARED_MEM_NAME);
#endif
    
    printf("Parent process finished\n");
    return EXIT_SUCCESS;
}
