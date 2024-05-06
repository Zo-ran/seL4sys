#pragma once

#include <sel4utils/process.h>
#include "filesystem/fs.h"

#define MAX_FILE_NUM 32

struct PCB {
    sel4utils_process_t proc;
    seL4_Word heap_top;
    FCB file_table[MAX_FILE_NUM];
};
typedef struct PCB PCB;


PCB *pid_getproc(seL4_Word pid);
int alloc_pid();