#pragma once

void disk_init();
void read_disk(void *destBuffer, int size, int num, int offset);
void write_disk(void *destBuffer, int size, int num, int offset);