#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <utils/time.h>
#include <muslcsys/vsyscall.h>
#include <errno.h>
#include "ipc_wrapper.h"

long sys_nanosleep(va_list ap) {
    struct timespec *req = va_arg(ap, struct timespec *);
    /* construct a sleep call */
    int micros = req->tv_sec * US_IN_S;
    micros += req->tv_nsec / NS_IN_US;
    syscall_ipc_normal(2, SYSCALL_SLEEP, micros);
    return 0;
}

long sys_clock_gettime(va_list ap) {
    clockid_t clk_id = va_arg(ap, clockid_t);
    struct timespec *res = va_arg(ap, struct timespec *);
    if (clk_id != CLOCK_REALTIME) {
        return -EINVAL;
    }
    syscall_ipc_normal(1, SYSCALL_GETIME);
    int64_t micros = seL4_GetMR(0);
    res->tv_sec = micros / US_IN_S;
    res->tv_nsec = (micros % US_IN_S) * NS_IN_US;
    return 0;
}