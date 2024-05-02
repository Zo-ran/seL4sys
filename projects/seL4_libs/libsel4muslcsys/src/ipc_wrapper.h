#pragma once

#include <sel4/sel4.h>

#define SERVER_EP_BADGE 0x8

seL4_MessageInfo_t syscall_ipc_normal(seL4_Uint32 length, ...);
seL4_MessageInfo_t syscall_ipc_write(const char *str, seL4_Uint64 len);