#ifndef CROSS_PLATFORM_H
#define CROSS_PLATFORM_H

#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

typedef struct {
    void* data;
    size_t size;
#ifdef _WIN32
    HANDLE hFile;
    HANDLE hMap;
#else
    int fd;
#endif
} mmap_file_t;

#ifdef _WIN32
typedef struct {
    HANDLE handle;
    HANDLE hStdInput;
    HANDLE hStdOutput;
} process_t;
#else
typedef struct {
    pid_t pid;
    int stdin_fd;
    int stdout_fd;
} process_t;
#endif

int  CpMmapCreate(mmap_file_t* mmap, const char* name, size_t size);
int  CpMmapOpen(mmap_file_t* mmap, const char* name, size_t size);
void CpMmapClose(mmap_file_t* mmap);
void CpMmapSync(mmap_file_t* mmap);

int  CpProcessCreate(process_t* proc, const char* path);
int  CpProcessWrite(process_t* proc, const char* data, size_t size);
int  CpProcessRead(process_t* proc, char* buffer, size_t size);
int  CpProcessClose(process_t* proc);

size_t CpStringLength(const char* str);
int    CpStringContains(const char* str, const char* substr);

static inline const char* CpGetChildProcessName(const char* baseName) {
    (void)baseName;
#ifdef _WIN32
    return "child.exe";
#else
    return "./child";
#endif
}

#define SHARED_MEM_NAME "lab3_shared_memory"
#define SHARED_MEM_SIZE 4096

#endif