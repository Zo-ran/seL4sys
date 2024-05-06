#include "process.h"

int cur_pid;

#define MAX_PROCESS_NUMBER 16

PCB proc_table[MAX_PROCESS_NUMBER];

PCB *pid_getproc(seL4_Word pid) {
    assert(pid < MAX_PROCESS_NUMBER);
    return proc_table + pid;
}

int alloc_pid() {   
    cur_pid += 1;
    assert(cur_pid < MAX_PROCESS_NUMBER);
    return cur_pid;
}