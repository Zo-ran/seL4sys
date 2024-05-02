#include <muslcsys/vsyscall.h>
#include <stdarg.h>
#include <assert.h>
#include "ipc_wrapper.h"
#include "arch_stdio.h"


seL4_MessageInfo_t syscall_ipc_normal(seL4_Uint32 length, ...) {
    va_list ap;
    va_start(ap, length);
    seL4_MessageInfo_t info_tag = seL4_MessageInfo_new(0, 0, 0, length);
    for (int i = 0; i < length; ++i)
        seL4_SetMR(i, va_arg(ap, seL4_Word));
    return seL4_Call(SERVER_EP_BADGE, info_tag);
}

seL4_MessageInfo_t syscall_ipc_write(const char *str, seL4_Uint64 len) {
    seL4_Uint32 msg_len = len + 2;
    assert(msg_len <= seL4_MsgMaxLength);
    seL4_MessageInfo_t info_tag = seL4_MessageInfo_new(0, 0, 0, msg_len);
    seL4_SetMR(0, SYSCALL_WRITE);
    seL4_SetMR(1, len);
    for (int i = 0; i < len; ++i) {
        seL4_SetMR(i + 2, str[i]);
    }
    return seL4_Call(SERVER_EP_BADGE, info_tag);
}