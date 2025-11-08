#include "crossplatform.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32

int CpMmapCreate(mmap_file_t* mmap, const char* name, size_t size) {
    if (!mmap || !name) return -1;
    
    mmap->hFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        (DWORD)size,
        name
    );
    
    if (mmap->hFile == NULL) return -1;
    
    mmap->data = MapViewOfFile(mmap->hFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (mmap->data == NULL) {
        CloseHandle(mmap->hFile);
        return -1;
    }
    
    mmap->size = size;
    memset(mmap->data, 0, size);
    return 0;
}

int CpMmapOpen(mmap_file_t* mmap, const char* name, size_t size) {
    if (!mmap || !name) return -1;
    
    mmap->hFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);
    if (mmap->hFile == NULL) return -1;
    
    mmap->data = MapViewOfFile(mmap->hFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
    if (mmap->data == NULL) {
        CloseHandle(mmap->hFile);
        return -1;
    }
    
    mmap->size = size;
    return 0;
}

void CpMmapClose(mmap_file_t* mmap) {
    if (!mmap) return;
    
    if (mmap->data) {
        UnmapViewOfFile(mmap->data);
        mmap->data = NULL;
    }
    
    if (mmap->hFile) {
        CloseHandle(mmap->hFile);
        mmap->hFile = NULL;
    }
}

void CpMmapSync(mmap_file_t* mmap) {
    if (mmap && mmap->data) {
        FlushViewOfFile(mmap->data, mmap->size);
    }
}

#else

int CpMmapCreate(mmap_file_t* mmap, const char* name, size_t size) {
    if (!mmap || !name) return -1;
    
    mmap->fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (mmap->fd == -1) return -1;
    
    if (ftruncate(mmap->fd, size) == -1) {
        close(mmap->fd);
        return -1;
    }
    
    mmap->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap->fd, 0);
    if (mmap->data == MAP_FAILED) {
        close(mmap->fd);
        return -1;
    }
    
    mmap->size = size;
    memset(mmap->data, 0, size);
    return 0;
}

int CpMmapOpen(mmap_file_t* mmap, const char* name, size_t size) {
    if (!mmap || !name) return -1;
    
    mmap->fd = shm_open(name, O_RDWR, 0666);
    if (mmap->fd == -1) return -1;
    
    mmap->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mmap->fd, 0);
    if (mmap->data == MAP_FAILED) {
        close(mmap->fd);
        return -1;
    }
    
    mmap->size = size;
    return 0;
}

void CpMmapClose(mmap_file_t* mmap) {
    if (!mmap) return;
    
    if (mmap->data && mmap->data != MAP_FAILED) {
        munmap(mmap->data, mmap->size);
        mmap->data = NULL;
    }
    
    if (mmap->fd != -1) {
        close(mmap->fd);
        mmap->fd = -1;
    }
}

void CpMmapSync(mmap_file_t* mmap) {
    if (mmap && mmap->data && mmap->data != MAP_FAILED) {
        msync(mmap->data, mmap->size, MS_SYNC);
    }
}

#endif

#ifdef _WIN32

int CpProcessCreate(process_t* proc, const char* path) {
    if (!proc || !path) return -1;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    HANDLE childStdoutRead = NULL;
    HANDLE childStdoutWrite = NULL;
    HANDLE childStdinRead = NULL;
    HANDLE childStdinWrite = NULL;

    if (!CreatePipe(&childStdoutRead, &childStdoutWrite, &sa, 0)) return -1;
    if (!CreatePipe(&childStdinRead, &childStdinWrite, &sa, 0)) {
        CloseHandle(childStdoutRead);
        CloseHandle(childStdoutWrite);
        return -1;
    }

    SetHandleInformation(childStdoutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(childStdinWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.hStdError = childStdoutWrite;
    si.hStdOutput = childStdoutWrite;
    si.hStdInput = childStdinRead;
    si.dwFlags |= STARTF_USESTDHANDLES;

    char cmdline[1024];
    strncpy(cmdline, path, sizeof(cmdline)-1);
    cmdline[sizeof(cmdline)-1] = '\0';

    if (!CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(childStdoutRead);
        CloseHandle(childStdoutWrite);
        CloseHandle(childStdinRead);
        CloseHandle(childStdinWrite);
        return -1;
    }

    CloseHandle(childStdoutWrite);
    CloseHandle(childStdinRead);

    proc->handle = pi.hProcess;
    proc->hStdInput = childStdinWrite;
    proc->hStdOutput = childStdoutRead;

    CloseHandle(pi.hThread);
    return 0;
}

int CpProcessWrite(process_t* proc, const char* data, size_t size) {
    if (!proc || !data) return -1;
    DWORD written = 0;
    if (!WriteFile(proc->hStdInput, data, (DWORD)size, &written, NULL)) {
        return -1;
    }
    return (int)written;
}

int CpProcessRead(process_t* proc, char* buffer, size_t size) {
    if (!proc || !buffer || size == 0) return -1;
    DWORD readBytes = 0;
    if (!ReadFile(proc->hStdOutput, buffer, (DWORD)(size - 1), &readBytes, NULL)) {
        return -1;
    }
    buffer[readBytes] = '\0';
    return (int)readBytes;
}

int CpProcessClose(process_t* proc) {
    if (!proc) return -1;
    int exitCode = -1;
    if (proc->hStdInput) {
        CloseHandle(proc->hStdInput);
        proc->hStdInput = NULL;
    }
    if (proc->hStdOutput) {
        CloseHandle(proc->hStdOutput);
        proc->hStdOutput = NULL;
    }
    if (proc->handle) {
        WaitForSingleObject(proc->handle, INFINITE);
        DWORD code;
        if (GetExitCodeProcess(proc->handle, &code)) {
            exitCode = (int)code;
        }
        CloseHandle(proc->handle);
        proc->handle = NULL;
    }
    return exitCode;
}

#else

int CpProcessCreate(process_t* proc, const char* path) {
    if (!proc || !path) return -1;
    int inpipe[2];
    int outpipe[2];

    if (pipe(inpipe) == -1) return -1;
    if (pipe(outpipe) == -1) {
        close(inpipe[0]); close(inpipe[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(inpipe[0]); close(inpipe[1]);
        close(outpipe[0]); close(outpipe[1]);
        return -1;
    }

    if (pid == 0) {
        dup2(inpipe[0], STDIN_FILENO);
        dup2(outpipe[1], STDOUT_FILENO);

        close(inpipe[0]); close(inpipe[1]);
        close(outpipe[0]); close(outpipe[1]);

        execl(path, "child", NULL);
        _exit(127);
    } else {
        close(inpipe[0]);
        close(outpipe[1]);
        proc->pid = pid;
        proc->stdin_fd = inpipe[1];
        proc->stdout_fd = outpipe[0];
        return 0;
    }
}

int CpProcessWrite(process_t* proc, const char* data, size_t size) {
    if (!proc || !data) return -1;
    ssize_t n = write(proc->stdin_fd, data, size);
    if (n == -1) return -1;
    return (int)n;
}

int CpProcessRead(process_t* proc, char* buffer, size_t size) {
    if (!proc || !buffer || size == 0) return -1;
    ssize_t n = read(proc->stdout_fd, buffer, (ssize_t)(size - 1));
    if (n == -1) return -1;
    if (n == 0) {
        buffer[0] = '\0';
        return 0;
    }
    buffer[n] = '\0';
    return (int)n;
}

int CpProcessClose(process_t* proc) {
    if (!proc) return -1;
    int status = -1;
    if (proc->stdin_fd != -1) {
        close(proc->stdin_fd);
        proc->stdin_fd = -1;
    }
    if (proc->stdout_fd != -1) {
        close(proc->stdout_fd);
        proc->stdout_fd = -1;
    }
    if (proc->pid > 0) {
        waitpid(proc->pid, &status, 0);
        if (WIFEXITED(status)) return WEXITSTATUS(status);
    }
    return -1;
}

#endif