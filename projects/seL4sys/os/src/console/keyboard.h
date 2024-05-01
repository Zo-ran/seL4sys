#pragma once

#include <allocman/vka.h>

#define MAX_KEYBUFFER_SIZE 64

extern char key_buffer[MAX_KEYBUFFER_SIZE];
extern int buffer_head;
extern int buffer_tail;
extern vka_object_t buf_full_ntfn;

void kbd_init(void);
void kbd_irq_handle_mainloop();