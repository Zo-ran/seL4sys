// seL4 lib
#include <sel4/sel4.h>
#include <muslcsys/vsyscall.h>
#include <arch_stdio.h>
#include <vm_layout.h>
#include <shared_area.h>

// C standard lib
#include <unistd.h>

#include "syscall_handler.h"
#include "console/keyboard.h"
#include "console/vga.h"
#include "filesystem/fs.h"
#include "timer.h"
#include "process.h"

void handle_syscall(seL4_MessageInfo_t msg_tag, bool *have_reply, seL4_MessageInfo_t *reply_tag, seL4_Word badge) {
    seL4_Word syscall_number = seL4_GetMR(0);
    *have_reply = true;
    PCB *sender_pcb = pid_getproc(badge);
    
    switch (syscall_number) {
        case SYSCALL_READ: {
            int fileno = seL4_GetMR(1), max_len = seL4_GetMR(2), read_len = 0;
            char *read_data = get_shared_area(max_len);
            if (fileno == STDIN_FILENO) {
                seL4_Wait(buf_full_ntfn.cptr, NULL);
                while (buffer_head != buffer_tail && read_len < max_len) {
                    read_data[read_len] = key_buffer[buffer_head++];
                    buffer_head %= MAX_KEYBUFFER_SIZE;
                    read_len += 1;
                }
            } else {
                read_len = syscall_readfile(sender_pcb->file_table + fileno, fileno, read_data, max_len);
            }
            seL4_SetMR(0, read_len);
            seL4_SetMR(1, (seL4_Word)read_data);
            break;
        }
        case SYSCALL_WRITE: {
            int fileno = seL4_GetMR(1), len = seL4_GetMR(2), write_len;
            const char *data = (char *)seL4_GetMR(3);
            if (fileno == STDOUT_FILENO || fileno == STDERR_FILENO) {
                for (int i = 0; i < len; ++i) {
                    char ch = data[i];
                    __arch_putchar(ch); 
                    vga_putchar(ch, true);
                }
                update_cursor();
                write_len = len;
            } else {
                write_len = syscall_writefile(sender_pcb->file_table + fileno, fileno, data, len);
            }
            seL4_SetMR(0, write_len);
            break;
        }
        case SYSCALL_SLEEP:
            timer_sleep(seL4_GetMR(1));
            break;
        case SYSCALL_GETIME:
            seL4_SetMR(0, timer_get_time());
            break;
        case SYSCALL_BRK: {
            seL4_Word new_brk = seL4_GetMR(1), ret;
            if (new_brk == 0) {
                ret = HEAP_BASE_VADDR;
                sender_pcb->heap_top = ret;
            } else if (sender_pcb->heap_top <= new_brk) {
                while (sender_pcb->heap_top < new_brk) {
                    void *vaddr = (void *) sender_pcb->heap_top;
                    reservation_t reserve = vspace_reserve_range_at(&sender_pcb->proc.vspace, vaddr, PAGE_SIZE_4K, seL4_AllRights, 1);
                    sender_pcb->heap_top += PAGE_SIZE_4K;
                    VFrame *vframe = (VFrame *)malloc(sizeof(VFrame));
                    vframe->reserve = reserve;
                    vframe->dirty = 0;
                    vframe->accessed = 0;
                    vframe->pagefile = NULL;
                    vframe->inRAM = 0;
                    vframe->next = sender_pcb->proc.vframes;
                    sender_pcb->proc.vframes = vframe;
                }
                ret = sender_pcb->heap_top;
            } else {
                assert(!"munmap currently not implemented");
            }
            seL4_SetMR(0, ret);
            break;
        }
        case SYSCALL_OPEN: {
            const char *path = (char *)seL4_GetMR(1);
            int flags = seL4_GetMR(2);
            int inodeOffset = syscall_open(path, flags);
            if (inodeOffset == -1) {
                seL4_SetMR(0, -1);
                return;
            }
            for (int i = 3; i < MAX_FILE_NUM; i++)
                if (!sender_pcb->file_table[i].inuse) {
                    sender_pcb->file_table[i].inuse = 1;
                    sender_pcb->file_table[i].inodeOffset = inodeOffset;
                    sender_pcb->file_table[i].offset = 0;
                    sender_pcb->file_table[i].flags = flags;
                    seL4_SetMR(0, i);
                    break;
                }
            break;
        }
        case SYSCALL_CLOSE: {
            int fd = seL4_GetMR(1);
            if (fd >= MAX_FILE_NUM || fd < 0)
                seL4_SetMR(0, -1);
            else
                sender_pcb->file_table[fd].inuse = 0;
            break;
        }
        case SYSCALL_UNLINK: {
            const char *path = (char *)seL4_GetMR(1);
            seL4_SetMR(0, syscall_unlink(path));
            break;
        }
        case SYSCALL_LSEEK: {
            int fileno = seL4_GetMR(1), offset = seL4_GetMR(2), whence = seL4_GetMR(3);
            seL4_SetMR(0, syscall_lseek(sender_pcb->file_table + fileno, fileno, offset, whence));
            break;
        }
        default:
            assert(0);
            *have_reply = false;
    }
}