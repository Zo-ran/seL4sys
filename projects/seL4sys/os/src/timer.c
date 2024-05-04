#include "timer.h"
#include "utils.h"
#include <platsupport/timer.h>
#include <sel4platsupport/io.h>

ltimer_t timer;
ps_io_ops_t timer_ops;
vka_object_t timer_ntfn_object;

void timer_init(vka_t *vka, vspace_t *vspace, simple_t *simple) {
    int error = vka_alloc_notification(vka, &timer_ntfn_object);
    error = sel4platsupport_new_malloc_ops(&timer_ops.malloc_ops);
    error = sel4platsupport_new_io_mapper(vspace, vka, &timer_ops.io_mapper);
    error = sel4platsupport_new_fdt_ops(&timer_ops.io_fdt, simple, &timer_ops.malloc_ops);
    error = sel4platsupport_new_mini_irq_ops(&timer_ops.irq_ops, vka, simple, &timer_ops.malloc_ops, timer_ntfn_object.cptr, MASK(seL4_BadgeBits));
    error = sel4platsupport_new_arch_ops(&timer_ops, simple, vka);
    error = ltimer_default_init(&timer, timer_ops, NULL, NULL);
    error = ltimer_set_timeout(&timer, NS_IN_MS, TIMEOUT_PERIODIC);
    assert(error == 0);
}

void timer_sleep(seL4_Word microsec) {
    seL4_Word count = 0, milisec = microsec / 1000;
    while (count < milisec) {
        seL4_Word badge;
        seL4_Wait(timer_ntfn_object.cptr, &badge);
        sel4platsupport_irq_handle(&timer_ops.irq_ops, MINI_IRQ_INTERFACE_NTFN_ID, badge);
        count++;
    }
}

uint64_t timer_get_time() {
    uint64_t nanosec;
    ltimer_get_time(&timer, &nanosec);
    return nanosec / 1000;
}