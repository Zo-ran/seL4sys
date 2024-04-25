#include "vga.h"
#include "rootvars.h"

#define VGA_PADDR 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEM_SIZE 0x1000

static uint16_t *VGA_MEM;
static uint16_t VGA_BUFFER[VGA_WIDTH * VGA_HEIGHT];

int displayRow = 0;
int displayCol = 0;

static inline void clear_screen() {
    for (int i = 0; i < VGA_WIDTH; ++i)
        for (int j = 0; j < VGA_HEIGHT; ++j)
            VGA_MEM[j * VGA_WIDTH + i] = 0 | (0xc << 8);
}
static inline void scroll_screen() {
	for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++)
        VGA_MEM[i] = VGA_MEM[i + VGA_WIDTH];
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; ++i)
        VGA_MEM[i] = 0 | (0xc << 8);
}

static inline void update_cursor(){
	int cursorPos = displayRow * VGA_WIDTH + displayCol;
	outByte(0x3d4, 0x0f);
	outByte(0x3d5, (unsigned char)(cursorPos & 0xff));
	outByte(0x3d4, 0x0e);
	outByte(0x3d5, (unsigned char)((cursorPos>>8) & 0xff));
}

static inline void vga_pos_putchar(int row, int col, char ch) {
    VGA_MEM[row * VGA_WIDTH + col] = ((u_int16_t) ch) | (0xc << 8);
}

static inline void check_displayrow() {
    if (displayRow == VGA_HEIGHT) {
        scroll_screen();
        displayRow = VGA_HEIGHT - 1;
        displayCol = 0;
    }   
}

static inline void check_displaycol() {
    if (displayCol == VGA_WIDTH){
        displayCol = 0;
        displayRow += 1;
        check_displayrow();
    }
}

void vga_init() {
    VGA_MEM = ps_io_map(&io_ops.io_mapper, VGA_PADDR, VGA_MEM_SIZE, false, PS_MEM_NORMAL);
    clear_screen();
    update_cursor();
}

void vga_putchar(char ch) {
    if (ch != 0) {
        if (ch == '\n') {
            displayCol = 0;
            displayRow += 1;
            check_displayrow();        
        } else {
            // write video memory
            VGA_MEM[displayRow * VGA_WIDTH + displayCol] = ((u_int16_t) ch) | (0xc << 8);    
            displayCol += 1;
            check_displaycol();
        }
    }
    update_cursor();
}
