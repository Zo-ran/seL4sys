#include "process.h"

int cur_pid;

#define MAX_PROCESS_NUMBER 16

sel4utils_process_t proc_table[MAX_PROCESS_NUMBER];

sel4utils_process_t *pid_getproc(seL4_Word pid) {
    assert(pid < MAX_PROCESS_NUMBER);
    return proc_table + pid;
}

int alloc_pid() {
    cur_pid += 1;
    assert(cur_pid < MAX_PROCESS_NUMBER);
    return cur_pid;
}