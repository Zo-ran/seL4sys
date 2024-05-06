#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include "ext.h"

SuperBlock sBlock;
GroupDesc gDesc[MAX_GROUP_NUM];

// #define O_RDONLY	     00        0x0               0
// #define O_WRONLY	     01        0x1               1
// #define O_RDWR        02        0x2              10
// #define O_CREAT       0100     0x40         1000000
// #define O_APPEND      02000
// #define O_DIRECTORY 0200000

// #define O_EXCL         0200
// #define O_NOCTTY       0400
// #define O_TRUNC       01000
// #define O_NONBLOCK    04000
// #define O_DSYNC      010000
// #define O_SYNC     04010000
// #define O_RSYNC    04010000
// #define O_NOFOLLOW  0400000
// #define O_CLOEXEC  02000000


static inline int stringChrR(const char *string, char token, int *size) {
	int i = 0;
	if (string == NULL) {
		*size = 0;
		return -1;
	}
	while (string[i] != 0)
		i ++;
	*size = i;
	while (i > -1) {
		if (token == string[i]) {
			*size = i;
			return 0;
		}
		else
			i --;
	}
	return -1;
}

void ls(const char *filename) {
    Inode inode;
	int inodeOffset = 0;
    int ret = readInode(&sBlock, gDesc, &inode, &inodeOffset, filename);
    assert(ret == 0);
    DirEntry dir;
    for (int i = 0; getDirEntry(&sBlock, &inode, i, &dir) == 0; ++i) {
        printf("%s\n", dir.name);
    }
}

int syscall_open(const char *path, int mode) {
    Inode fatherInode, destInode;
	int fatherInodeOffset = 0, destInodeOffset = 0;
    int ret, destlen = strlen(path), fatherlen = 0;
    char str[NAME_LENGTH] = "\0";
    strcpy(str, path);
    if (readInode(&sBlock, gDesc, &destInode, &destInodeOffset, path) != 0) {
        // File not exists 
        // Create it!
        if (!(mode & O_CREAT))
            return -1;
        if (mode & O_CREAT) {
            if (str[destlen - 1] == '/') {
                if (mode & O_DIRECTORY) {
                    str[destlen - 1] = 0;
                    destlen -= 1;
                } else {
                    return -1;
                }
            }
            char father_str[NAME_LENGTH] = "\0";
            stringChrR(str, '/', &fatherlen);
            if (fatherlen == 0)
                father_str[0] = '/';
            else
                strncpy(father_str, str, fatherlen);
            if (readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, father_str) != 0) 
                return -1;
            int type = (mode & O_DIRECTORY) ? DIRECTORY_TYPE : REGULAR_TYPE;
            ret = allocInode(&sBlock, gDesc, &fatherInode, fatherInodeOffset, &destInode, &destInodeOffset, str + fatherlen + 1, type);
            assert(ret == 0);
        }
    }
    return destInodeOffset;
}

void filesystem_init() {
	readGroupHeader(&sBlock, gDesc);
    // int c = open("/fuck", O_CREAT);
    ls("/");
    // printf("open ret: %d\n", c);
	printf("%d\n", sBlock.sectorNum); 
	printf("%d\n", sBlock.inodeNum);
	printf("%d\n", sBlock.availInodeNum);
	printf("%d\n", sBlock.availBlockNum);
    printf("%p %p %p %p %p\n", O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_DIRECTORY);
}