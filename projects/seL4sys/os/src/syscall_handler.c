// seL4 lib
#include <sel4/sel4.h>
#include <muslcsys/vsyscall.h>
#include <arch_stdio.h>
#include <vm_layout.h>

// C standard lib
#include <unistd.h>

#include "syscall_handler.h"
#include "console/keyboard.h"
#include "console/vga.h"
#include "timer.h"
#include "process.h"

void handle_syscall(seL4_MessageInfo_t msg_tag, bool *have_reply, seL4_MessageInfo_t *reply_tag, seL4_Word badge) {
    seL4_Word syscall_number = seL4_GetMR(0);
    *have_reply = true;
    switch (syscall_number) {
        case SYSCALL_READ: {
            int fileno = seL4_GetMR(1), max_len = seL4_GetMR(2), read_len = 0;
            if (fileno == STDIN_FILENO) {
                seL4_Wait(buf_full_ntfn.cptr, NULL);
                while (buffer_head != buffer_tail && read_len < max_len) {
                    seL4_SetMR(read_len, key_buffer[buffer_head++]);
                    buffer_head %= MAX_KEYBUFFER_SIZE;
                    read_len += 1;
                }
                seL4_MessageInfo_ptr_set_length(reply_tag, read_len);
            } else {
                assert(!"Filesystem not implemented!\n");
            }
            break;
        }
        case SYSCALL_WRITE: {
            seL4_Word len = seL4_GetMR(1);
            for (int i = 0; i < len; ++i) {
                char ch = seL4_GetMR(i + 2);
                __arch_putchar(ch); 
                vga_putchar(ch, true);
            }
            update_cursor();
            seL4_MessageInfo_ptr_set_length(reply_tag, 1);
            seL4_SetMR(0, len);
            break;
        }
        case SYSCALL_SLEEP:
            timer_sleep(seL4_GetMR(1));
            break;
        case SYSCALL_GETIME:
            seL4_MessageInfo_ptr_set_length(reply_tag, 1);
            seL4_SetMR(0, timer_get_time());
            break;
        case SYSCALL_BRK: {
            seL4_Word new_brk = seL4_GetMR(1), ret;
            PCB *sender_pcb = pid_getproc(badge);
            assert(badge == 1);
            if (new_brk == 0) {
                ret = HEAP_BASE_VADDR;
                sender_pcb->heap_top = ret;
            } else if (sender_pcb->heap_top <= ret) {
                while (sender_pcb->heap_top < ret) {
                    void *vaddr = (void *) sender_pcb->heap_top;
                    reservation_t reserve = vspace_reserve_range_at(&sender_pcb->proc.vspace, vaddr, PAGE_SIZE_4K, seL4_AllRights, 1);
                    assert(reserve.res != NULL);
                    int error = vspace_new_pages_at_vaddr(&sender_pcb->proc.vspace, vaddr, 1, PAGE_BITS_4K, reserve);
                    assert(error == 0);
                    sender_pcb->heap_top += PAGE_SIZE_4K;
                }
                ret = sender_pcb->heap_top;
            } else {
                assert(!"munmap currently not implemented");
            }
            seL4_SetMR(0, ret);
            break;
        }
        case SYSCALL_OPEN: {
            char *pathname = seL4_GetMR(1);
            int flags = seL4_GetMR(2);
            printf("name: %s flags: %p\n", pathname, flags);
            assert(0);
        }
        default:
            assert(0);
            *have_reply = false;
    }
}