#pragma once

#include "ext.h"

typedef struct Pagefile Pagefile;
struct Pagefile {
    char name[32];
    int inuse;
    Pagefile *next;
};

void pagefile_init(SuperBlock *sBlock, GroupDesc *gDesc);
void write_page_to_file(const char *filename);
void read_file_to_page(const char *filename);
char *get_pagefile();