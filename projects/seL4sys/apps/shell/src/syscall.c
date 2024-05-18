#include <sel4/sel4.h>
#include <stdarg.h>
#include <muslcsys/vsyscall.h>
#include <stdio.h>

#define SERVER_EP_BADGE 0x8

static seL4_MessageInfo_t syscall_ipc_normal(seL4_Uint32 length, ...) {
    va_list ap;
    va_start(ap, length);
    seL4_MessageInfo_t info_tag = seL4_MessageInfo_new(0, 0, 0, length);
    for (int i = 0; i < length; ++i)
        seL4_SetMR(i, va_arg(ap, seL4_Word));
    return seL4_Call(SERVER_EP_BADGE, info_tag);
}

void sys_ls(const char *path) {
    char buf[64] = "\0";
    syscall_ipc_normal(3, SYSCALL_LS, path, buf);
    printf("%s\n", buf);
}

void sys_ps() {
    char buf[1024] = "\0";
    syscall_ipc_normal(2, SYSCALL_PS, buf);
    printf("%s\n", buf);
}