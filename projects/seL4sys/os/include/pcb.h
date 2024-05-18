#pragma once

#include <sel4utils/process.h>
#include "fcb.h"

#define MAX_CWD_LEN 32

struct PCB {
    int inuse;
    int stime;
    sel4utils_process_t proc;
    seL4_Word heap_top;
    FCB file_table[MAX_FILE_NUM];
    char cwd[MAX_CWD_LEN];
    char cmd[MAX_CWD_LEN];
};
typedef struct PCB PCB;