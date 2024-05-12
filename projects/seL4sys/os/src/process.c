#include "process.h"
#include "rootvars.h"
#include "filesystem/pagefile.h"

#include <fcntl.h>
#include <unistd.h>

#define MAX_PROCESS_NUM 16
#define MAX_ELF_SIZE 1024 * 1024

PCB proc_table[MAX_PROCESS_NUM];

PCB *pid_getproc(seL4_Word pid) {
    assert(pid < MAX_PROCESS_NUM);
    return proc_table + pid;
}

int alloc_pid() {   
    for (int i = 0; i < MAX_PROCESS_NUM; ++i)
        if (!proc_table[i].inuse) {
            proc_table[i].inuse = 1;
            return i;
        }
    assert(0);
    return -1;
}

int syscall_execve(const char *path, int argc, char **argv) {
    // read elf file
    char elf_file[MAX_ELF_SIZE];
    int elf_size;
    FCB vfcb;
    vfcb.flags = O_RDONLY;
    vfcb.offset = 0;
    vfcb.inuse = 1;
    vfcb.inodeOffset = syscall_open(path, O_RDONLY);
    elf_size = syscall_readfile(&vfcb, 3, elf_file, MAX_ELF_SIZE);

    int cur_pid = alloc_pid();
    PCB *new_pcb = pid_getproc(cur_pid);
    sel4utils_process_t *new_process = &new_pcb->proc;
    new_process->pframes_used = 0;
    new_process->vframes = NULL;
    sel4utils_process_config_t config = process_config_default_simple(&simple, "", 1);
    cspacepath_t cap_path;
    config.create_fault_endpoint = false;
    config.fault_endpoint = fault_ep;
    sel4utils_configure_process_custom(new_process, &vka, &vspace, config, cur_pid, elf_file, elf_size);

    NAME_THREAD(new_process->thread.tcb.cptr, argv[0]);
    vka_cspace_make_path(&vka, syscall_ep, &cap_path);
    sel4utils_mint_cap_to_process(new_process, cap_path, seL4_AllRights, cur_pid);

    for (int i = 0; i < 3; ++i) {
        new_pcb->file_table[i].inuse = 1;
        new_pcb->file_table[i].inodeOffset = -1;
        new_pcb->file_table[i].offset = 0;
        new_pcb->file_table[i].flags = O_WRONLY;
    }
    new_pcb->file_table[STDIN_FILENO].flags = O_RDONLY;
    new_process->replacer = new_process->vframes;
    // start new process
    sel4utils_spawn_process_v(new_process, &vka, &vspace, argc, argv, seL4_MaxPrio);
    return cur_pid;
}

int syscall_kill(int pid, int sig) {
    if (!proc_table[pid].inuse)
        return -1;
    if (sig == 9 || sig == 15) {
        sel4utils_destroy_process(&proc_table[pid].proc, &vka);

        // free the pagefile process used
        for (VFrame *vframe = proc_table[pid].proc.vframes; vframe != NULL; vframe = vframe->next)
            if (vframe->pagefile != NULL)
                free_pagefile(vframe->pagefile);
        memset(proc_table + pid, 0, sizeof(PCB));
        printf("exit success!\n");
        return 0;
    }
    return -1;
}