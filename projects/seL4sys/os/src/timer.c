#include "timer.h"
#include "rootvars.h"
#include "utils.h"

ltimer_t timer;
ps_io_ops_t timer_ops;
vka_object_t timer_ntfn_object;

void timer_init(void) {
    
    FUNC_IFERR("Failed to allocate notification", vka_alloc_notification, &vka, &timer_ntfn_object);
    
    FUNC_IFERR("", sel4platsupport_new_malloc_ops, &timer_ops.malloc_ops);
    FUNC_IFERR("", sel4platsupport_new_io_mapper, &vspace, &vka, &timer_ops.io_mapper);
    FUNC_IFERR("", sel4platsupport_new_fdt_ops, &timer_ops.io_fdt, &simple, &timer_ops.malloc_ops);
    FUNC_IFERR("", sel4platsupport_new_mini_irq_ops, &timer_ops.irq_ops, &vka, &simple, &timer_ops.malloc_ops,
                                                 timer_ntfn_object.cptr, MASK(seL4_BadgeBits));
    FUNC_IFERR("", sel4platsupport_new_arch_ops, &timer_ops, &simple, &vka);

    FUNC_IFERR("Failed to init timer", ltimer_default_init, &timer, timer_ops, NULL, NULL);
    FUNC_IFERR("Failed to set timer timeout", ltimer_set_timeout, &timer, NS_IN_S, TIMEOUT_PERIODIC);
}

void func() {
    int count = 0;
    while (1) {
        seL4_Word badge;
        seL4_Wait(timer_ntfn_object.cptr, &badge);
        sel4platsupport_irq_handle(&timer_ops.irq_ops, MINI_IRQ_INTERFACE_NTFN_ID, badge);
        count++;
        if (count == 5) {
            break;
        }
    }
}