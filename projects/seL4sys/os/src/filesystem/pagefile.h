#pragma once

#include "../../include/ext.h"

typedef struct Pagefile Pagefile;
struct Pagefile {
    char name[32];
    int inuse;
    Pagefile *next;
};

void pagefile_init(SuperBlock *sBlock, GroupDesc *gDesc);
void write_page_to_file(const char *filename, void *vaddr);
void read_file_to_page(const char *filename, void *vaddr);
char *get_pagefile();
void free_pagefile(const char *filename);