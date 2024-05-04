#pragma once

#include <sel4utils/process.h>

sel4utils_process_t *pid_getproc(seL4_Word pid);
int alloc_pid();