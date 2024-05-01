#pragma once

void vga_init(void);
void vga_putchar(char ch, int Mwaterline);
void vga_delchar();
void update_cursor();