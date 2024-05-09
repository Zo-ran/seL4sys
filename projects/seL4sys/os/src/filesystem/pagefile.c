#include "pagefile.h"
#include "fs.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
        strcpy(pf->name, dir.name);
        pf->inuse = 0;
        pf->next = pagefiles;

        pagefiles = pf;
        pagefile_num += 1;
    }
}

char *get_pagefile() {
    Pagefile *pf = pagefiles;
    printf("pagefile: %d\n", pagefile_num);
    for (; pf != NULL; pf = pf->next)
        if (!pf->inuse)
            break;
    if (pf != NULL) {
        pf->inuse = 1;
        return pf->name;
    } else {
        Pagefile *pf = (Pagefile *)malloc(sizeof(Pagefile));
        pf->inuse = 1;
        sprintf(pf->name, "page%d", pagefile_num);
        pf->next = pagefiles;

        pagefiles = pf;
        pagefile_num += 1;

        char buf[NAME_LENGTH] = "\0";
        sprintf(buf, "%s/%s", PAGEFILE_PATH, pf->name);
        syscall_open(buf, O_CREAT);
        return pf->name;
    }
}