// seL4 Lib
#include <simple-default/simple-default.h>
#include <allocman/bootstrap.h>

#include <sel4platsupport/bootinfo.h>
#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/arch/io.h>
#include <sel4platsupport/irq.h>

#include <sel4utils/thread.h>
#include <sel4utils/vspace.h>
#include <sel4utils/process.h>
#include <sel4utils/vspace_internal.h>
#include <sel4utils/helpers.h>

#include <sel4runtime.h>

#include <utils/util.h>

#include <muslcsys/vsyscall.h>


// C standard Lib
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>

// My Lib
#include "utils.h"
#include "ioport.h"
#include "syscall_handler.h"
#include "console/keyboard.h"
#include "console/vga.h"
#include "filesystem/disk.h"
#include "filesystem/fs.h"
#include "vm/vmfault_handler.h"
#include "timer/timer.h"
#include "process.h"

seL4_BootInfo *boot_info;
simple_t simple;
vka_t vka;
allocman_t *allocman;
vspace_t vspace;
ps_io_ops_t io_ops;
vka_object_t fault_ep;
seL4_CPtr syscall_ep;
seL4_CPtr root_cspace_cap;
seL4_CPtr root_vspace_cap;

/* static memory for the allocator to bootstrap with */
#define ALLOCATOR_STATIC_POOL_SIZE (BIT(seL4_PageBits) * 10000)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

/* static memory for virtual memory bootstrapping */
static sel4utils_alloc_data_t data;

/* IRQ I/O ports */
#define PORT_PIC_MASTER 0x20
#define PORT_PIC_SLAVE  0xA0
#define IRQ_SLAVE       2


// TODO: add lock to shared memory
// TODO: set different sys server threads

void rootvars_init() {
    boot_info = platsupport_get_bootinfo();
    simple_default_init_bootinfo(&simple, boot_info);   
    allocman = bootstrap_use_current_simple(&simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    ZF_LOGF_IF(allocman == NULL, "Failed to initialize alloc manager!\n");
    allocman_make_vka(&vka, allocman);
    root_cspace_cap = simple_get_cnode(&simple);
    root_vspace_cap = simple_get_pd(&simple);
    FUNC_IFERR("Failed to create vspace object!\n", sel4utils_bootstrap_vspace_with_bootinfo_leaky, &vspace, &data, root_vspace_cap, &vka, boot_info);

    void *vaddr;
    reservation_t virtual_reservation;
    virtual_reservation = vspace_reserve_range(&vspace,
                                               ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    ZF_LOGF_IF(virtual_reservation.res == NULL, "Failed to reserve a chunk of memory!\n");
    bootstrap_configure_virtual_pool(allocman, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE, root_vspace_cap);

    FUNC_IFERR("Failed to init I/O operations!\n", sel4platsupport_new_io_ops, &vspace, &vka, &simple, &io_ops);
    FUNC_IFERR("Failed to init I/O port operations!\n", sel4platsupport_new_arch_ops, &io_ops, &simple, &vka);
}

void syscallserver_ipc_init() {
    vka_object_t obj = {0};
    FUNC_IFERR("Failed to allocate endpoint!\n", vka_alloc_endpoint, &vka, &obj);
    FUNC_IFERR("Failed to allocate endpoint!\n", vka_alloc_endpoint, &vka, &fault_ep);
    syscall_ep = obj.cptr;
}


uint8_t inByte(uint16_t port) {
	uint32_t data;
    io_ops.io_port_ops.io_port_in_fn(io_ops.io_port_ops.cookie, port, 1, &data);
	return data;
}

void outByte(uint16_t port, uint8_t data) {
    io_ops.io_port_ops.io_port_out_fn(io_ops.io_port_ops.cookie, port, 1, data);
}

uint32_t inLong(uint16_t port) {
    uint32_t data;
    io_ops.io_port_ops.io_port_in_fn(io_ops.io_port_ops.cookie, port, 4, &data);
	return data;
}

void outLong(uint16_t port, uint32_t data) {
    io_ops.io_port_ops.io_port_out_fn(io_ops.io_port_ops.cookie, port, 4, data);
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

void handle_syscall_loop() {
    bool have_reply = false;
    while (1) {
        seL4_Word sender_badge;
        seL4_MessageInfo_t msg_tag, reply_tag;
        if (have_reply) {
            msg_tag = seL4_ReplyRecv(syscall_ep, reply_tag, &sender_badge);
        } else {
            msg_tag = seL4_Recv(syscall_ep, &sender_badge);
        }
        handle_syscall(msg_tag, &have_reply, &reply_tag, sender_badge);
    }
}

void load_shell(const char *app_name, uint8_t app_prio) {
    int cur_pid = alloc_pid();
    PCB *new_pcb = pid_getproc(cur_pid);
    sel4utils_process_t *new_process = &new_pcb->proc;
    sel4utils_process_config_t config = process_config_default_simple(&simple, app_name, app_prio);
    cspacepath_t cap_path;
    config.create_fault_endpoint = false;
    config.fault_endpoint = fault_ep;
    // configure user test process
    FUNC_IFERR("Failed to configure new process!\n", sel4utils_configure_process_custom, new_process, &vka, &vspace, config, cur_pid, NULL, 0);
    NAME_THREAD(new_process->thread.tcb.cptr, "user test thread");

    // mint the syscall endpoint into the process
    vka_cspace_make_path(&vka, syscall_ep, &cap_path);
    sel4utils_mint_cap_to_process(new_process, cap_path, seL4_AllRights, cur_pid);

    // set up process stdin stdout stderr TODO: complete it
    for (int i = 0; i < 3; ++i) {
        new_pcb->file_table[i].inuse = 1;
        new_pcb->file_table[i].inodeOffset = -1;
        new_pcb->file_table[i].offset = 0;
        new_pcb->file_table[i].flags = O_WRONLY;
    }
    new_pcb->file_table[STDIN_FILENO].flags = O_RDONLY;
    
    // set up process syscall shared memory 
    reservation_t reserve = vspace_reserve_range_at(&new_process->vspace, (void *)SYS_SHARED_AREA_VADDR, PAGE_SIZE_4M, seL4_AllRights, 1);
    vspace_share_mem_at_vaddr(&vspace, &new_process->vspace, (void *)SYS_SHARED_AREA_VADDR, 1, PAGE_BITS_4M, (void *)SYS_SHARED_AREA_VADDR, reserve);
    new_process->replacer = new_process->vframes;
    // start new process
    FUNC_IFERR("Failed to start new process!\n", sel4utils_spawn_process_v, new_process, &vka, &vspace, 0, NULL, 1);
}

void sys_shared_area_setup() {
    reservation_t reserve = vspace_reserve_range_at(&vspace, (void *)SYS_SHARED_AREA_VADDR, PAGE_SIZE_4M, seL4_AllRights, 1);
    int error = vspace_new_pages_at_vaddr(&vspace, (void *)SYS_SHARED_AREA_VADDR, 1, PAGE_BITS_4M, reserve);
    assert(error == 0);
}

void start_system_thread(const char *name, sel4utils_thread_entry_fn entry_point, void *arg0, void *arg1, void *arg2) {
    vka_object_t tcb;
    seL4_CPtr ipc_frame;
    seL4_UserContext regs = {0};
    size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);
    FUNC_IFERR("Failed to alloc tcb!\n", vka_alloc_tcb, &vka, &tcb);
    NAME_THREAD(tcb.cptr, name);
    
    void *vaddr = vspace_new_pages(&vspace, seL4_AllRights, 1, PAGE_BITS_4K);
    ipc_frame = vspace_get_cap(&vspace, vaddr);

    FUNC_IFERR("Failed to configure new TCB!\n", seL4_TCB_Configure, tcb.cptr, seL4_CapNull, root_cspace_cap, seL4_NilData, root_vspace_cap, seL4_NilData, (seL4_Word) vaddr, ipc_frame);

    void *stack = vspace_new_pages(&vspace, seL4_AllRights, 1, PAGE_BITS_4K);
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    uintptr_t stack_top = (uintptr_t)stack + PAGE_SIZE_4K;
    ZF_LOGF_IF(stack_top % (stack_alignment_requirement) != 0, "Stack top isn't aligned correctly to a %dB boundary.\n", stack_alignment_requirement);

    sel4utils_arch_init_local_context(entry_point, arg0, arg1, arg2, (void *)stack_top, &regs);
    FUNC_IFERR("Failed to write to the thread's registers!\n", seL4_TCB_WriteRegisters, tcb.cptr, 0, 0, regs_size, &regs);

    void *tls_mem = vspace_new_pages(&vspace, seL4_AllRights, 1, PAGE_BITS_4K);

    uintptr_t tls = sel4runtime_write_tls_image(tls_mem);
    seL4_IPCBuffer *ipcbuf = (seL4_IPCBuffer*)vaddr;
    FUNC_IFERR("Failed to set ipc buffer in TLS of new thread!\n", sel4runtime_set_tls_variable, tls, __sel4_ipc_buffer, ipcbuf);
    FUNC_IFERR("Failed to set TLS base", seL4_TCB_SetTLSBase, tcb.cptr, tls);

    // set priority
    FUNC_IFERR("Failed to set the priority!\n", seL4_TCB_SetPriority, tcb.cptr, seL4_CapInitThreadTCB, seL4_MaxPrio);

    // start kbd thread
    FUNC_IFERR("Failed to new thread!\n", seL4_TCB_Resume, tcb.cptr);
}

void test() {
    seL4_Word vaddr = 0x200d;
    printf("test: %p\n", PAGE_ALIGN_4K(vaddr));
    printf("oooook\n");
    while (1);
}

int main(int argc, char *argv[]) {
    // init useful variables for rootserver
    rootvars_init();

    // setup serial and irq
    serial_init();
    interrupt_init();

    // init device drivers
    vga_init(&io_ops.io_mapper);
    timer_init(&vka, &vspace, &simple);
    kbd_init(&vka, &simple);
    disk_init();
    filesystem_init();
    sys_shared_area_setup();

    // init syscall endpoint
    syscallserver_ipc_init();

    // load user apps
    // test();
    load_shell("shell", 1);

    // start keyboard irq handle thread
    start_system_thread("kbd irq handler", kbd_irq_handle_mainloop, 0, 0, 0);
    start_system_thread("vmfault handler", handle_vmfault_loop, (void *)fault_ep.cptr, 0, 0);

    seL4_DebugDumpScheduler();

    // wait and handle syscall
    handle_syscall_loop();
    return 0;
}