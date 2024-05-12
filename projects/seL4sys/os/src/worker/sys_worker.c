#include "sys_worker.h"
#include <sel4/sel4.h>

typedef struct {
    seL4_CPtr ntfn;
    bgworker_callback_fn fn;
    void* data;
} SysWorker;

SysWorker workers[64];

