#pragma once

#include <stdarg.h>

void _init_syscall_table(void);

long _sys_set_tid_address(va_list ap);
long _sys_exit(va_list ap);
long _sys_rt_sigprocmask(va_list ap);
long _sys_gettid(va_list ap);
long _sys_getpid(va_list ap);
long _sys_tgkill(va_list ap);
long _sys_exit_group(va_list ap);
long _sys_openat(va_list ap);
long _sys_close(va_list ap);
long _sys_readv(va_list ap);
long _sys_read(va_list ap);
long _sys_ioctl(va_list ap);
long _sys_brk(va_list ap);
long _sys_mmap(va_list ap);
long _sys_writev(va_list ap);
long _sys_write(va_list ap);
long _sys_nanosleep(va_list ap);
long _sys_clock_gettime(va_list ap);