#pragma once

#include <sel4/sel4.h>
#include "../../include/fcb.h"
#include "../../include/pcb.h"

typedef struct FileData {
    seL4_CPtr reply;
    PCB *pcb;
    FCB *fcb;
    int fd;
    char *str;
    int max_len;
} FileData;

void filesystem_init();
int syscall_open(const char *path, int mode);
int syscall_unlink(const char *path);
int syscall_readfile(FCB *fcb, int fd, char *str, int max_len);
int syscall_writefile(FCB *fcb, int fd, const char *str, int len);
int syscall_lseek(FCB *fcb, int fd, int offset, int whence);
void readfile_handler(void *data);
void writefile_handler(void *data);
void openfile_handler(void *data);
void unlink_handler(void *data);
void lseek_handler(void *data);
void ls(const char *filename, char *buf);