#include "sys_worker.h"
#include "../rootvars.h"

#include <sel4/sel4.h>
#include <stdio.h>

#define MAX_WORKER_NUM 8

typedef struct {
    int busy;
    seL4_CPtr ntfn;
    bgworker_callback_fn fn;
    void* data;
} SysWorker;

SysWorker workers[MAX_WORKER_NUM];

SysWorker *get_idle_worker() {
    for (int i = 0; i < MAX_WORKER_NUM; ++i)
        if (!workers[i].busy)
            return workers + i;
    return NULL;
}

void sysworker_dispatch_thread(bgworker_callback_fn fn, void *data) {
    // get a worker dispatch request
    SysWorker *idle_worker;
    while ((idle_worker = get_idle_worker()) == NULL);
    idle_worker->busy = 1;
    idle_worker->fn = fn;
    idle_worker->data = data;
    seL4_Signal(idle_worker->ntfn);
}


void sysworker_loop(void *number, void *unuse0, void *unuse1) {
    SysWorker *sysworker = workers + (seL4_Word)number;
    while (1) {
        seL4_Wait(sysworker->ntfn, NULL);
        assert(sysworker->fn != NULL);
        sysworker->fn(sysworker->data);
        sysworker->busy = 0;
    }
}

void sysworkers_init() {
    for (int i = 0; i < MAX_WORKER_NUM; ++i) {
        vka_object_t ntfn;
        vka_alloc_endpoint(&vka, &ntfn);
        workers[i].busy = 0;
        workers[i].ntfn = ntfn.cptr;
        workers[i].fn = NULL;
        workers[i].data = NULL;
        char buf[16] = "\0";
        sprintf(buf, "worker %d", i);
        start_system_thread(buf, 253, sysworker_loop, (void *)i, 0, 0);
    }
}