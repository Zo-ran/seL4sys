#pragma once

#include <sel4utils/process.h>
#include "filesystem/fs.h"

#define MAX_FILE_NUM 32

struct PCB {
    int inuse;
    sel4utils_process_t proc;
    seL4_Word heap_top;
    FCB file_table[MAX_FILE_NUM];
};
typedef struct PCB PCB;

typedef struct FileData {
    seL4_CPtr reply;
    PCB *pcb;
    FCB *fcb;
    int fd;
    char *str;
    int max_len;
} FileData;


PCB *pid_getproc(seL4_Word pid);
int alloc_pid();
int syscall_execve(const char *path, int argc, char **argv);
int syscall_kill(int pid, int sig);