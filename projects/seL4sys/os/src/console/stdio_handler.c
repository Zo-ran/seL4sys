#include "keyboard.h"
#include "stdio_handler.h"
#include "../rootvars.h"
#include "../vm/mem_projection.h"
#include "vga.h"


#include <arch_stdio.h>

// sync_bin_sem_t stdio_sem;

void stdio_sem_init() {
    // sync_bin_sem_new(&vka, &stdio_sem, 1);
}

void readstdin_handler(void *data) {
    StdioData *sd = (StdioData *)data;
    int max_len = sd->max_len, read_len = 0;
    buffer_tail = buffer_head;
    seL4_Wait(buf_full_ntfn.cptr, NULL);
    char *read_data = (char *)setup_projection_space(sd->pcb, (void *)sd->wdata);
    assert(!cross_page((void *)read_data, (void *)(read_data + sd->max_len)));
    while (buffer_head != buffer_tail && read_len < max_len) {
        read_data[read_len] = key_buffer[buffer_head++];
        buffer_head %= MAX_KEYBUFFER_SIZE;
        read_len += 1;
    }
    seL4_SetMR(0, read_len);
    seL4_NBSend(sd->reply, seL4_MessageInfo_new(0, 0, 0, 2));
    destroy_projection_space((void *)read_data);
    vka_cspace_free(&vka, sd->reply);
    free(data);
}

void writestdout_handler(void *data) {
    StdioData *sd = (StdioData *)data;
    const char *wdata = (char *)setup_projection_space(sd->pcb, (void *)sd->wdata);
    assert(!cross_page((void *)wdata, (void *)(wdata + sd->max_len)));
    for (int i = 0; i < sd->max_len; ++i) {
        char ch = wdata[i];
        __arch_putchar(ch); 
        vga_putchar(ch, true);
    }
    update_cursor();
    seL4_SetMR(0, sd->max_len);
    seL4_NBSend(sd->reply, seL4_MessageInfo_new(0, 0, 0, 1));
    destroy_projection_space((void *)wdata);
    vka_cspace_free(&vka, sd->reply);
    free(data);
}