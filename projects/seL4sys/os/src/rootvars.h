#pragma once

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
#include <sel4platsupport/platsupport.h>
#include <platsupport/plat/timer.h>
#include <platsupport/ltimer.h>

#include <arch_stdio.h>

#include <keyboard/keyboard.h>
#include <keyboard/codes.h>

extern seL4_BootInfo *boot_info;
extern simple_t simple;
extern vka_t vka;
extern allocman_t *allocman;
extern vspace_t vspace;
extern seL4_CPtr syscall_ep;
extern ps_io_ops_t io_ops;

uint8_t inByte(uint16_t port);
void outByte(uint16_t port, uint8_t data);