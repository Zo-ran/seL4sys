#include <stdio.h>
#include <stdlib.h>
#include <utils/util.h>
#include <muslcsys/vsyscall.h>
#include <syscalls.h>
#include <sel4runtime.h>

void _init_syscall_table(void)
{
    setbuf(stdout, NULL);
    muslcsys_install_syscall(__NR_set_tid_address, _sys_set_tid_address);
    muslcsys_install_syscall(__NR_writev, _sys_writev);
    muslcsys_install_syscall(__NR_write, _sys_write);
    muslcsys_install_syscall(__NR_exit, _sys_exit);
    muslcsys_install_syscall(__NR_rt_sigprocmask, _sys_rt_sigprocmask);
    muslcsys_install_syscall(__NR_gettid, _sys_gettid);
    muslcsys_install_syscall(__NR_getpid, _sys_getpid);
    muslcsys_install_syscall(__NR_tgkill, _sys_tgkill);
    muslcsys_install_syscall(__NR_exit_group, _sys_exit_group);
    muslcsys_install_syscall(__NR_ioctl, _sys_ioctl);
    muslcsys_install_syscall(__NR_mmap, _sys_mmap);
    muslcsys_install_syscall(__NR_brk,  _sys_brk);
    muslcsys_install_syscall(__NR_clock_gettime, _sys_clock_gettime);
    muslcsys_install_syscall(__NR_nanosleep, _sys_nanosleep);
    muslcsys_install_syscall(__NR_openat, _sys_openat);
    muslcsys_install_syscall(__NR_close, _sys_close);
    muslcsys_install_syscall(__NR_readv, _sys_readv);
    muslcsys_install_syscall(__NR_read, _sys_read);
}
