#pragma once

#define MAX_FILE_NUM 32

struct FCB {
    int inuse;       
    int inodeOffset; 
    int offset;      
    int flags;
};
typedef struct FCB FCB;