// C standard Lib
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

// My Lib
#include <syscalls.h>
#include "utils.h"
#include "rootvars.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"

seL4_BootInfo *boot_info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;
ps_io_ops_t io_ops;
seL4_CPtr syscall_ep;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10)
UNUSED static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

/* static memory for virtual memory bootstrapping */
UNUSED static sel4utils_alloc_data_t data;

void rootvars_init() {
    boot_info = platsupport_get_bootinfo();
    simple_default_init_bootinfo(&simple, boot_info);   
    // simple_print(&simple);
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize alloc manager.\n");
    allocman_make_vka(&vka, allocman);
    // FUNC_IFERR("Failed to get I/O port operations", sel4platsupport_get_io_port_ops, &ioport_ops, &simple, &vka);
    FUNC_IFERR("Failed to create vspace object", sel4utils_bootstrap_vspace_with_bootinfo_leaky, &vspace,
                                                           &data, simple_get_pd(&simple), &vka, boot_info);
    FUNC_IFERR("Failed to init I/O operations", sel4platsupport_new_io_ops, &vspace, &vka, &simple, &io_ops);
    FUNC_IFERR("Failed to init I/O port operations", sel4platsupport_new_arch_ops, &io_ops, &simple, &vka);
}

void ipc_init() {
    vka_object_t obj = {0};
    FUNC_IFERR("Failed to allocate endpoint", vka_alloc_endpoint, &vka, &obj);
    syscall_ep = obj.cptr;
}

#define PORT_PIC_MASTER 0x20
#define PORT_PIC_SLAVE  0xA0
#define IRQ_SLAVE       2

uint8_t inByte(uint16_t port) {
	uint32_t data;
    io_ops.io_port_ops.io_port_in_fn(io_ops.io_port_ops.cookie, port, 1, &data);
	return data;
}

void outByte(uint16_t port, uint8_t data) {
    io_ops.io_port_ops.io_port_out_fn(io_ops.io_port_ops.cookie, port, 1, data);
}

void serial_init() {
    platsupport_serial_setup_simple(&vspace, &simple, &vka);
}

void interrupt_init(void) {
	outByte(PORT_PIC_MASTER, 0x11); // ICW1, Initialization command
	outByte(PORT_PIC_SLAVE, 0x11); // ICW1, Initialization command
	outByte(PORT_PIC_MASTER + 1, 32); // ICW2, Interrupt Vector Offset 0x20
	outByte(PORT_PIC_SLAVE + 1, 32 + 8); // ICW2, Interrupt Vector Offset 0x28
	outByte(PORT_PIC_MASTER + 1, 1 << 2); // ICW3, Tell Master PIC that there is a slave
	outByte(PORT_PIC_SLAVE + 1, 2); // ICW3, Tell Slave PIC its cascade identity
	outByte(PORT_PIC_MASTER + 1, 0x3); // ICW4, Auto EOI in 8086/88 mode
	outByte(PORT_PIC_SLAVE + 1, 0x3); // ICW4, Auto EOI in 8086/88 mode
}

int main(int argc, char *argv[]) {
    NAME_THREAD(seL4_CapInitThreadTCB, "root");
    // _init_syscall_table();
    rootvars_init();
    serial_init();
    interrupt_init();
    vga_init();
    kbd_init();
    ipc_init();
    timer_init();
    printf("Successfully init rootserver!\n");
    return 0;
}