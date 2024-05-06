#pragma once

struct FCB {
    int inuse;       
    int inodeOffset; 
    int offset;      
    int flags;
};
typedef struct FCB FCB;

void filesystem_init();