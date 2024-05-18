#include "pagefile.h"
#include "fs.h"
#include "ext.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <utils/page.h>

#define PAGEFILE_PATH "/swap"

Pagefile *pagefiles;
int pagefile_num;

void pagefile_init(SuperBlock *sBlock, GroupDesc *gDesc) {
    pagefiles = NULL;
    pagefile_num = 0;

    Inode inode;
	int inodeOffset = 0;
    readInode(sBlock, gDesc, &inode, &inodeOffset, PAGEFILE_PATH);
    DirEntry dir;
    for (int i = 0; getDirEntry(sBlock, &inode, i, &dir) == 0; ++i) {
        Pagefile *pf = (Pagefile *)malloc(sizeof(Pagefile));
        sprintf(pf->name, "%s/%s", PAGEFILE_PATH, dir.name);
        pf->inuse = 0;
        pf->next = pagefiles;

        pagefiles = pf;
        pagefile_num += 1;
    }
}

char *get_pagefile() {
    Pagefile *pf = pagefiles;
    for (; pf != NULL; pf = pf->next)
        if (!pf->inuse)
            break;
    if (pf != NULL) {
        pf->inuse = 1;
        return pf->name;
    } else {
        Pagefile *pf = (Pagefile *)malloc(sizeof(Pagefile));
        pf->inuse = 1;
        sprintf(pf->name, "%s/page%d", PAGEFILE_PATH, pagefile_num);
        pf->next = pagefiles;

        pagefiles = pf;
        pagefile_num += 1;

        syscall_open(pf->name, O_CREAT);
        return pf->name;
    }
}

void write_page_to_file(const char *filename, void *vaddr) {
    FCB vfcb;
    vfcb.flags = O_RDWR;
    vfcb.offset = 0;
    vfcb.inuse = 1;
    vfcb.inodeOffset = syscall_open(filename, O_RDWR);
    syscall_writefile(&vfcb, 3, vaddr, PAGE_SIZE_4K);
}

void read_file_to_page(const char *filename, void *vaddr) {
    FCB vfcb;
    vfcb.flags = O_RDWR;
    vfcb.offset = 0;
    vfcb.inuse = 1;
    vfcb.inodeOffset = syscall_open(filename, O_RDWR);
    int ret = syscall_readfile(&vfcb, 3, vaddr, PAGE_SIZE_4K);
    assert(ret == PAGE_SIZE_4K);
}

void free_pagefile(const char *filename) {
    for (Pagefile *pf = pagefiles; pf != NULL; pf = pf->next)
        if (strcmp(pf->name, filename) == 0) {
            pf->inuse = 0;
            return;
        }
}