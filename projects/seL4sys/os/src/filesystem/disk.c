#include <stdio.h>
#include "ext.h"
#include "../ioport.h"

void wait_disk(void) {
	while((inByte(0x1F7) & 0xC0) != 0x40) {};
}

void read_sect(void *dst, int offset) {
	int i;
	wait_disk();
	
	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x20);

	wait_disk();
	for (i = 0; i < SECTOR_SIZE / 4; i ++) {
		((uint32_t *)dst)[i] = inLong(0x1F0);
	}
}

void write_sect(void *src, int offset) {
	int i;
	wait_disk();

	outByte(0x1F2, 1);
	outByte(0x1F3, offset);
	outByte(0x1F4, offset >> 8);
	outByte(0x1F5, offset >> 16);
	outByte(0x1F6, (offset >> 24) | 0xE0);
	outByte(0x1F7, 0x30);

	wait_disk();
	for (i = 0; i < SECTOR_SIZE / 4; i ++) {
		outLong(0x1F0, ((uint32_t *)src)[i]);
	}
}

void read_disk(void *destBuffer, int size, int num, int offset) {
	int i = 0;
	int j = 0;
	uint8_t buffer[SECTOR_SIZE];
	int quotient = offset / SECTOR_SIZE;
	int remainder = offset % SECTOR_SIZE;
	
	read_sect((void*)buffer, quotient + j);
	j++;
	while (i < size * num) {
		((uint8_t*)destBuffer)[i] = buffer[(remainder + i) % SECTOR_SIZE];
		i++;
		if ((remainder + i) % SECTOR_SIZE == 0) {
			read_sect((void*)buffer, quotient + j);
			j++;
		}
	}
}

void write_disk(void *destBuffer, int size, int num, int offset) {
	int i = 0;
	int j = 0;
	uint8_t buffer[SECTOR_SIZE];
	int quotient = offset / SECTOR_SIZE;
	int remainder = offset % SECTOR_SIZE;

	read_sect((void*)buffer, quotient + j);
	while (i < size * num) {
		buffer[(remainder + i) % SECTOR_SIZE] = ((uint8_t*)destBuffer)[i];
		i++;
		if ((remainder + i) % SECTOR_SIZE == 0) {
			write_sect((void*)buffer, quotient + j);
			j++;
			read_sect((void*)buffer, quotient + j);
		}
	}
	write_sect((void*)buffer, quotient + j);
}

void disk_init() {
	outByte(0x1F6, 0xA0);
    wait_disk();
    // outByte(0x1F6, 0xA0);
    // outByte(0x1F7, 0xEC);
	// wait_disk();


    // char str[1024] = "\0";
    // char data[512] = "helloworld";
	// for (int i = 0; i < 100; ++i) {
	// 	sprintf(data, "this time is %d", i);s
	// write_sect(data, (1 << 20) - 1);
	// read_sect(str, (1 << 20) - 1);
	// printf("debug: %s\n", str);
	// }
	// for (int i = 0; i < 100; ++i) {
	// 	read_sect(str, 0);
    // 	printf("debug: %s\n", str);
	// }
	// for (int i = 0; i < 100; ++i) {
	// 	write_sect(str, 0);
    // 	// printf("debug: %s\n", str);
	// }
}