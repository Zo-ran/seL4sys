#pragma once

#define SECTOR_NUM 8196
#define SECTOR_SIZE 512

void disk_init();
void read_disk(void *destBuffer, int size, int num, int offset);
void write_disk(void *destBuffer, int size, int num, int offset);