#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include "ext.h"
#include "fs.h"
#include "pagefile.h"
#include "../rootvars.h"
#include "../process.h"
#include "../vm/mem_projection.h"

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

// TODO: add relative path support
// TODO: implement active inode table
// TODO: set file priviledge level

void ls(const char *filename) {
    Inode inode;
	int inodeOffset = 0;
    int ret = readInode(&sBlock, gDesc, &inode, &inodeOffset, filename);
    assert(ret == 0);
    DirEntry dir;
    for (int i = 0; getDirEntry(&sBlock, &inode, i, &dir) == 0; ++i) {
        printf("%s ", dir.name);
    }
    printf("\n");
}

static inline int find_last_of(const char *str, char token, int end) {
    int last_index = -1, len = strlen(str);
    end = end < len ? end : (len - 1);
    for (int i = end; i >= 0; --i)
        if (str[i] == token)
            return i;
    return -1;
}

static inline void get_father_path(const char *src, char *father) {
    int src_len = strlen(src);
    int last_pos = find_last_of(src, '/', src_len - 2);
    if (last_pos == 0) {
        father[0] = '/';
        father[1] = 0;
    } else {
        strncpy(father, src, last_pos);
        father[last_pos] = 0;
    }
}

static inline int is_dir_path(const char *path) {
    return (path[strlen(path) - 1] == '/');
}

static inline int is_dir_mode(int mode) {
    return (mode & O_DIRECTORY);
}

static inline int is_empty_dir(Inode *inode) {
    DirEntry d;
    return (getDirEntry(&sBlock, inode, 0, &d) != 0);
}

static inline int mode_can_read(int mode) {
    return ((mode & 0x1) == 0);
}

static inline int  mode_can_write(int mode) {
    return ((mode & 0b11) != O_RDONLY);
}

static inline int min(int a, int b, int c) {
    return (a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c);
}

int syscall_open(const char *path, int mode) {
    Inode fatherInode, destInode;
	int fatherInodeOffset = 0, destInodeOffset = 0;
    int ret, destlen = strlen(path);
    if (readInode(&sBlock, gDesc, &destInode, &destInodeOffset, path) != 0) {
        // File not exists 
        // Create it!
        if (!(mode & O_CREAT))
            return -1;
        if (is_dir_path(path) && !is_dir_mode(mode))
            return -1;
        char father_path[NAME_LENGTH] = "\0";
        get_father_path(path, father_path);
        if (readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, father_path) != 0) 
            return -1;
        int type = is_dir_mode(mode) ? DIRECTORY_TYPE : REGULAR_TYPE;
        int last_slash = find_last_of(path, '/', destlen - 1);
        ret = allocInode(&sBlock, gDesc, &fatherInode, fatherInodeOffset, &destInode, &destInodeOffset, path + last_slash + 1, type);
        assert(ret == 0);
    }
    return destInodeOffset;
}

int syscall_unlink(const char *path) {
    Inode fatherInode, destInode;
	int fatherInodeOffset = 0, destInodeOffset = 0;
    if (readInode(&sBlock, gDesc, &destInode, &destInodeOffset, path) != 0)
        return -1;
    if (destInode.type == DIRECTORY_TYPE && !is_empty_dir(&destInode))
        return -1;
    char father_path[NAME_LENGTH] = "\0";
    get_father_path(path, father_path);
    if (readInode(&sBlock, gDesc, &fatherInode, &fatherInodeOffset, father_path) != 0)
        return -1;
    int last_slash = find_last_of(path, '/', strlen(path) - 1);
    return freeInode(&sBlock, gDesc, &fatherInode, fatherInodeOffset, &destInode, &destInodeOffset, path + last_slash + 1, destInode.type);
}

int syscall_readfile(FCB *fcb, int fd, char *str, int max_len) {
    if (fd < 3 || fd >= MAX_FILE_NUM || !fcb->inuse || !mode_can_read(fcb->flags))
        return -1;
    uint8_t buffer[sBlock.blockSize];
    int quotient = fcb->offset / sBlock.blockSize;
	int remainder = fcb->offset % sBlock.blockSize;
    Inode inode;
    read_disk(&inode, sizeof(Inode), 1, fcb->inodeOffset);
    if (inode.size <= fcb->offset)
        return 0;
    int read_len = 0;
    
    while (max_len > read_len && fcb->offset < inode.size) {
        readBlock(&sBlock, &inode, quotient++, buffer);
        int max_read_size = min(sBlock.blockSize - remainder, inode.size - fcb->offset, max_len - read_len);
        memcpy(str + read_len, buffer + remainder, max_read_size);
        fcb->offset += max_read_size;
        read_len += max_read_size;
        remainder = 0;
	}
    return read_len;
}

int syscall_writefile(FCB *fcb, int fd, const char *str, int len) {
    if (fd < 3 || fd >= MAX_FILE_NUM || !fcb->inuse || !mode_can_write(fcb->flags))
        return -1;
    uint8_t buffer[sBlock.blockSize];
    int quotient = fcb->offset / sBlock.blockSize;
	int remainder = fcb->offset % sBlock.blockSize;
    Inode inode;
    read_disk(&inode, sizeof(Inode), 1, fcb->inodeOffset);
    int write_len = 0;
    while (len > write_len) {
        if (quotient >= inode.blockCount) {
            assert(allocBlock(&sBlock, gDesc, &inode, fcb->inodeOffset) == 0);
        }
        readBlock(&sBlock, &inode, quotient, buffer);
        int max_write_size = (sBlock.blockSize - remainder) < (len - write_len) ? (sBlock.blockSize - remainder) : (len - write_len);
        memcpy(buffer + remainder, str + write_len, max_write_size);
        writeBlock(&sBlock, &inode, quotient, buffer);
        quotient += 1;
        write_len += max_write_size;
        fcb->offset += max_write_size;
        inode.size = inode.size > fcb->offset ? inode.size : fcb->offset;
        remainder = 0;
    }
    readBlock(&sBlock, &inode, 0, buffer);
    write_disk(&inode, sizeof(Inode), 1, fcb->inodeOffset);
    return len;
}

int syscall_lseek(FCB *fcb, int fd, int offset, int whence) {
    if (fd < 3 || fd >= MAX_FILE_NUM || !fcb->inuse)
        return -1;
    Inode inode;
    read_disk(&inode, sizeof(Inode), 1, fcb->inodeOffset);
    switch (whence) {
        case SEEK_SET: fcb->offset = offset; break;
        case SEEK_CUR: fcb->offset += offset; break;
        case SEEK_END: fcb->offset = inode.size + offset; break;
        default: return -1; 
    }
    return fcb->offset;
}

void filesystem_init() {
	readGroupHeader(&sBlock, gDesc);
    pagefile_init(&sBlock, gDesc);
    printf("available: %d\n", sBlock.availInodeNum);
    ls("/");
    ls("/swap");
}

void readfile_handler(void *data) {
    FileData *filed = (FileData *)data;
    FCB *fcb = filed->fcb;
    int fd = filed->fd, max_len = filed->max_len;
    char *str = (char *)setup_projection_space(filed->pcb, (void *)filed->str);
    assert(!cross_page((void *)str, (void *)(str + max_len)));
    seL4_SetMR(0, syscall_readfile(fcb, fd, str, max_len));
    seL4_SetMR(1, (seL4_Word)str);
    seL4_NBSend(filed->reply, seL4_MessageInfo_new(0, 0, 0, 2));
    destroy_projection_space((void *)str);
    vka_cspace_free(&vka, filed->reply);
    free(data);
}

void writefile_handler(void *data) {
    FileData *filed = (FileData *)data;
    FCB *fcb = filed->fcb;
    int fd = filed->fd, len = filed->max_len;
    const char *str = (char *)setup_projection_space(filed->pcb, (void *)filed->str);
    assert(!cross_page((void *)str, (void *)(str + len)));
    seL4_SetMR(0, syscall_writefile(fcb, fd, str, len));
    seL4_NBSend(filed->reply, seL4_MessageInfo_new(0, 0, 0, 1));
    destroy_projection_space((void *)str);
    vka_cspace_free(&vka, filed->reply);
    free(data);
}

void openfile_handler(void *data) {
    FileData *filed = (FileData *)data;
    const char *path = (char *)setup_projection_space(filed->pcb, (void *)filed->str);
    int mode = filed->max_len;

    int inodeOffset = syscall_open(path, mode);
    if (inodeOffset == -1) {
        seL4_SetMR(0, -1);
    } else {
        PCB *sender_pcb = filed->pcb;
        int i = 3;
        for (; i < MAX_FILE_NUM; i++)
            if (!sender_pcb->file_table[i].inuse) {
                sender_pcb->file_table[i].inuse = 1;
                sender_pcb->file_table[i].inodeOffset = inodeOffset;
                sender_pcb->file_table[i].offset = 0;
                sender_pcb->file_table[i].flags = mode;
                break;
            }
        seL4_SetMR(0, (i >= MAX_FILE_NUM ) ? -1 : i);
    }

    seL4_NBSend(filed->reply, seL4_MessageInfo_new(0, 0, 0, 1));
    destroy_projection_space((void *)path);
    vka_cspace_free(&vka, filed->reply);
    free(data);
}

void unlink_handler(void *data) {
    FileData *filed = (FileData *)data;
    const char *str = (char *)setup_projection_space(filed->pcb, (void *)filed->str);
    seL4_SetMR(0, syscall_unlink(str));
    seL4_NBSend(filed->reply, seL4_MessageInfo_new(0, 0, 0, 1));
    destroy_projection_space((void *)str);
    vka_cspace_free(&vka, filed->reply);
    free(data);
}

void lseek_handler(void *data) {
    FileData *filed = (FileData *)data;
    seL4_SetMR(0, syscall_lseek(filed->fcb, filed->fd, filed->max_len, (int)filed->str));
    seL4_NBSend(filed->reply, seL4_MessageInfo_new(0, 0, 0, 1));
    vka_cspace_free(&vka, filed->reply);
    free(data);
}