#pragma once

#include <sel4/sel4.h>
#include <allocman/vka.h>
#include <simple/simple.h>
#include <vspace/vspace.h>
#include <sel4platsupport/io.h>

extern seL4_BootInfo *boot_info;
extern simple_t simple;
extern vka_t vka;
extern allocman_t *allocman;
extern vspace_t vspace;
extern ps_io_ops_t io_ops;
extern seL4_CPtr syscall_ep;


uint8_t inByte(uint16_t port);
void outByte(uint16_t port, uint8_t data);