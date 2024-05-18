#pragma once

#include "../include/pcb.h"

PCB *pid_getproc(seL4_Word pid);
int alloc_pid();
int syscall_execve(PCB *parent, const char *path, int argc, char **argv);
int syscall_kill(int pid, int sig);
void syscall_ps(char *buf);