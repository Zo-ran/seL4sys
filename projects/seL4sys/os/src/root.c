// seL4 Lib
#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <utils/util.h>
#include <sel4runtime.h>

#include <vspace/vspace.h>

#include <sel4utils/sel4_zf_logif.h>
#include <sel4utils/thread.h>
#include <sel4utils/vspace.h>

#include <allocman/allocman.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <sel4platsupport/io.h>
#include <sel4platsupport/irq.h>
#include <sel4platsupport/arch/io.h>
#include <sel4platsupport/bootinfo.h>
#include <platsupport/plat/timer.h>
#include <platsupport/ltimer.h>

// C standard Lib
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

// My Lib
#include <syscalls.h>
#include "utils.h"

seL4_BootInfo *boot_info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;
ltimer_t timer;
vka_object_t timer_ntfn_object;
seL4_CPtr syscall_ep;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

void ipc_init(void) {
    vka_object_t obj = {0};
    FUNC_IFERR("Failed to allocate endpoint", vka_alloc_endpoint, &vka, &obj);
    syscall_ep = obj.cptr;
}

void vspace_init() {
    FUNC_IFERR("Failed to create vspace object", sel4utils_bootstrap_vspace_with_bootinfo_leaky, &vspace,
                                                           &data, simple_get_pd(&simple), &vka, boot_info);
    // void *vaddr;
    // UNUSED reservation_t virtual_reservation;
    // virtual_reservation = vspace_reserve_range(&vspace,
    //                                            ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    // assert(virtual_reservation.res);
    // bootstrap_configure_virtual_pool(allocman, vaddr,
    //                                  ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&simple));
}
ps_io_ops_t ops = {{0}};

void timer_init(void) {
    
    FUNC_IFERR("Failed to allocate notification", vka_alloc_notification, &vka, &timer_ntfn_object);
    
    FUNC_IFERR("", sel4platsupport_new_malloc_ops, &ops.malloc_ops);
    FUNC_IFERR("", sel4platsupport_new_io_mapper, &vspace, &vka, &ops.io_mapper);
    FUNC_IFERR("", sel4platsupport_new_fdt_ops, &ops.io_fdt, &simple, &ops.malloc_ops);
    FUNC_IFERR("", sel4platsupport_new_mini_irq_ops, &ops.irq_ops, &vka, &simple, &ops.malloc_ops,
                                                 timer_ntfn_object.cptr, MASK(seL4_BadgeBits));
    FUNC_IFERR("", sel4platsupport_new_arch_ops, &ops, &simple, &vka);

    FUNC_IFERR("Failed to init timer", ltimer_default_init, &timer, ops, NULL, NULL);
    FUNC_IFERR("Failed to set timer timeout", ltimer_set_timeout, &timer, NS_IN_S, TIMEOUT_PERIODIC);
}

// seL4_MessageInfo_t handle_syscall(UNUSED seL4_Word badge, UNUSED int num_args, bool *have_reply)
// {
//     seL4_MessageInfo_t reply_msg;

//     /* get the first word of the message, which in the SOS protocol is the number
//      * of the SOS "syscall". */
//     seL4_Word syscall_number = seL4_GetMR(0);

//     /* Set the reply flag */
//     *have_reply = true;

//     /* Process system call */
//     switch (syscall_number) {
//     case SOS_SYSCALL0:
//         ZF_LOGV("syscall: thread example made syscall 0!\n");
//         /* construct a reply message of length 1 */
//         reply_msg = seL4_MessageInfo_new(0, 0, 0, 1);
//         /* Set the first (and only) word in the message to 0 */
//         seL4_SetMR(0, 0);

//         break;
//     default:
//         reply_msg = seL4_MessageInfo_new(0, 0, 0, 0);
//         ZF_LOGE("Unknown syscall %lu\n", syscall_number);
//         /* Don't reply to an unknown syscall */
//         *have_reply = false;
//     }

//     return reply_msg;
// }

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

    // _init_syscall_table();
    vspace_init();
    ipc_init();
    timer_init();
    simple_default_init_bootinfo(&simple, boot_info);
    int count = 0;
    while (1) {
        seL4_Word badge;
        seL4_Wait(timer_ntfn_object.cptr, &badge);
        sel4platsupport_irq_handle(&ops.irq_ops, MINI_IRQ_INTERFACE_NTFN_ID, badge);
        count++;
        if (count == 5) {
            break;
        }
    }
    printf("Successfully init rootserver!\n");
    int a;
    syscall_loop(syscall_ep);
    return 0;
}