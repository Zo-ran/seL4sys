/*
EXT4 like filesystem
*/

/*
*                                                        Layout of block group 0
*+--------------------+--------------------+-------------------+--------------------+--------------------+--------------------+
*|     SuperBlock     |   GroupDescTable   |    InodeBitmap    |    BlockBitmap     |     InodeTable     |     DataBlocks     |
*+--------------------+--------------------+-------------------+--------------------+--------------------+--------------------+
*      1024 Bytes           Many Block            1 Block             1 Block             Many Blocks        Many More Blocks
*
*                                                        Layout of block group 1
*+--------------------+--------------------+-------------------+--------------------+--------------------+--------------------+
*|     SuperBlock     |   GroupDescTable   |    InodeBitmap    |    BlockBitmap     |     InodeTable     |     DataBlocks     |
*+--------------------+--------------------+-------------------+--------------------+--------------------+--------------------+
*      1024 Bytes           Many Block            1 Block             1 Block             Many Blocks        Many More Blocks

*/

/*
* GROUP_SIZE is at least 1024 bytes + 3 blocks
* s: SECTOR_NUM
* b: SECTORS_PER_BLOCK
* g: GROUP_NUM
* g = \lfloor\frac{s}{2+\lceil(32g/512/b)\rceil*b+b+b+\lceil(128*512*b/512/b)\rceil*b+512*b*b}\rfloor
*/

/* Independent Variables */
//#define SECTOR_NUM 1024 // i.e., 512 KB, as we take file to simulate hard disk, SECTOR_NUM is bounded by (2^32 / 2^9) sectors for 32-bits system
//#define SECTOR_NUM 8196 // defined in disk.h
//#define SECTOR_SIZE 512 // i.e., 512 B, defined in disk.h

#pragma once


#include <stdint.h>
#include "disk.h"
#include "../../include/ext.h"

int readGroupHeader(SuperBlock *superBlock, GroupDesc *groupDesc);
int readBlock (SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer);
int writeBlock (SuperBlock *superBlock, Inode *inode, int blockIndex, uint8_t *buffer);
int readInode (SuperBlock *superBlock, GroupDesc *groupDesc, Inode *destInode, int *inodeOffset, const char *destFilePath);
int allocBlock (SuperBlock *superBlock, GroupDesc *groupDesc, Inode *inode, int inodeOffset);
int allocInode (SuperBlock *superBlock, GroupDesc *groupDesc, Inode *fatherInode, int fatherInodeOffset, Inode *destInode, int *destInodeOffset, const char *destFilename, int destFiletype);
int freeInode (SuperBlock *superBlock, GroupDesc *groupDesc, Inode *fatherInode, int fatherInodeOffset, Inode *destInode, int *destInodeOffset, const char *destFilename, int destFiletype);
int getDirEntry (SuperBlock *superBlock, Inode *inode, int dirIndex, DirEntry *destDirEntry);