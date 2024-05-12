#pragma once

#include <sel4/sel4.h>
#include "../process.h"

typedef struct {
    PCB *pcb;
    seL4_CPtr reply;
    int max_len;
    const char *wdata;
} StdioData;

void readstdin_handler(void *data);
void writestdout_handler(void *data);
void stdio_sem_init();