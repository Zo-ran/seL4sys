// seL4 Lib
#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <sel4platsupport/bootinfo.h>

#include <utils/util.h>
#include <sel4runtime.h>

#include <sel4utils/sel4_zf_logif.h>
#include <sel4utils/thread.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

// C standard Lib
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

// My Lib
#include <syscalls.h>

seL4_BootInfo *boot_info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
seL4_CPtr syscall_ep;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

void ipc_init(void) {
    vka_object_t obj;
    int error = vka_alloc_endpoint(&vka, &obj);
    ZF_LOGF_IFERR(error, "Failed to allocate new endpoint object.\n");
    syscall_ep = obj.cptr;
}

NORETURN void syscall_loop(seL4_CPtr ep){
    while (1) {
        
    }
}

int main(int argc, char *argv[]) {
    boot_info = platsupport_get_bootinfo();
    simple_default_init_bootinfo(&simple, boot_info);   
    NAME_THREAD(seL4_CapInitThreadTCB, "root");

    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize alloc manager.\n");
    allocman_make_vka(&vka, allocman);

    _init_syscall_table();
    ipc_init();
    syscall_loop(syscall_ep);
    return 0;
}