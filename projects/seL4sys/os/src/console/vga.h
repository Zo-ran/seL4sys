#pragma once

#include <sel4platsupport/io.h>

void vga_init(const ps_io_mapper_t *io_mapper);
void vga_putchar(char ch, int Mwaterline);
void vga_delchar();
void update_cursor();