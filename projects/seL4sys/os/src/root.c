#include <sel4/sel4.h>

#include <simple/simple.h>
#include <simple-default/simple-default.h>

#include <sel4platsupport/bootinfo.h>

#include <muslcsys/vsyscall.h>

#include <stdio.h>
#include <stdlib.h>
#include <utils/util.h>
#include <sel4runtime.h>

// #include <syscalls.h>

seL4_BootInfo *boot_info;
simple_t simple;

int main(int argc, char *argv[]) {
    boot_info = platsupport_get_bootinfo();
    simple_default_init_bootinfo(&simple, boot_info);
    // _init_syscall_table();
    /* print out bootinfo and other info about simple */
    printf("fuck\n");
    int s;
    scanf("%d", &s);
    while (1);
    return 0;
}