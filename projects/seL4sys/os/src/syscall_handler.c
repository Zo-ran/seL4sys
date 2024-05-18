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
#include "console/stdio_handler.h"
#include "filesystem/fs.h"
#include "timer/timer.h"
#include "process.h"
#include "worker/sys_worker.h"
#include "rootvars.h"
#include "vm/mem_projection.h"

void handle_syscall(seL4_MessageInfo_t msg_tag, bool *have_reply, seL4_MessageInfo_t *reply_tag, seL4_Word badge) {
    seL4_Word syscall_number = seL4_GetMR(0);
    *have_reply = true;
    PCB *sender_pcb = pid_getproc(badge);
    seL4_CPtr reply;
    switch (syscall_number) {
        case SYSCALL_READ: {
            vka_cspace_alloc(&vka, &reply);
            seL4_CNode_SaveCaller(seL4_CapInitThreadCNode, reply, seL4_WordBits);
            int fileno = seL4_GetMR(1), max_len = seL4_GetMR(2), read_len = 0;
            char *data = (char *)seL4_GetMR(3);
            if (fileno == STDIN_FILENO) {
                StdioData *sd = (StdioData *)malloc(sizeof(StdioData));
                sd->reply = reply;
                sd->max_len = max_len;
                sd->wdata = data;
                sd->pcb = sender_pcb;
                sysworker_dispatch_thread(readstdin_handler, sd);
                *have_reply = false;
            } else {
                FileData *filed = (FileData *)malloc(sizeof(FileData));
                filed->pcb = sender_pcb;
                filed->reply = reply;
                filed->fcb = sender_pcb->file_table + fileno;
                filed->fd = fileno;
                filed->str = data;
                filed->max_len = max_len;
                sysworker_dispatch_thread(readfile_handler, filed);
                *have_reply = false;
            }
            break;
        }
        case SYSCALL_WRITE: {
            vka_cspace_alloc(&vka, &reply);
            seL4_CNode_SaveCaller(seL4_CapInitThreadCNode, reply, seL4_WordBits);
            int fileno = seL4_GetMR(1), len = seL4_GetMR(2);
            char *data = (char *)seL4_GetMR(3);
            if (fileno == STDOUT_FILENO || fileno == STDERR_FILENO) {
                StdioData *sd = (StdioData *)malloc(sizeof(StdioData));
                sd->reply = reply;
                sd->max_len = len;
                sd->wdata = data;
                sd->pcb = sender_pcb;
                sysworker_dispatch_thread(writestdout_handler, sd);
                *have_reply = false;
            } else {
                FileData *filed = (FileData *)malloc(sizeof(FileData));
                filed->reply = reply;
                filed->fcb = sender_pcb->file_table + fileno;
                filed->fd = fileno;
                filed->str = data;
                filed->max_len = len;
                filed->pcb = sender_pcb;
                sysworker_dispatch_thread(writefile_handler, filed);
                *have_reply = false;
            }
            break;
        }
        case SYSCALL_SLEEP: {
            vka_cspace_alloc(&vka, &reply);
            seL4_CNode_SaveCaller(seL4_CapInitThreadCNode, reply, seL4_WordBits);
            TimeData *td = (TimeData *)malloc(sizeof(TimeData));
            td->reply = reply;
            td->time = seL4_GetMR(1);
            sysworker_dispatch_thread(timer_sleep, td);
            *have_reply = false;
            break;
        }
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
            vka_cspace_alloc(&vka, &reply);
            seL4_CNode_SaveCaller(seL4_CapInitThreadCNode, reply, seL4_WordBits);
            char *path = (char *)seL4_GetMR(1);
            int flags = seL4_GetMR(2);

            FileData *filed = (FileData *)malloc(sizeof(FileData));
            filed->reply = reply;
            filed->str = path;
            filed->max_len = flags;
            filed->pcb = sender_pcb;
            sysworker_dispatch_thread(openfile_handler, filed);
            *have_reply = false;
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
            vka_cspace_alloc(&vka, &reply);
            seL4_CNode_SaveCaller(seL4_CapInitThreadCNode, reply, seL4_WordBits);
            char *path = (char *)seL4_GetMR(1);
            
            FileData *filed = (FileData *)malloc(sizeof(FileData));
            filed->pcb = sender_pcb;
            filed->reply = reply;
            filed->str = path;
            sysworker_dispatch_thread(unlink_handler, filed);
            *have_reply = false;
            break;
        }
        case SYSCALL_LSEEK: {
            vka_cspace_alloc(&vka, &reply);
            seL4_CNode_SaveCaller(seL4_CapInitThreadCNode, reply, seL4_WordBits);
            int fileno = seL4_GetMR(1), offset = seL4_GetMR(2), whence = seL4_GetMR(3);
            FileData *filed = (FileData *)malloc(sizeof(FileData));
            filed->reply = reply;
            filed->fd = fileno;
            filed->max_len = offset;
            filed->str = (char *)whence;
            filed->fcb = sender_pcb->file_table + fileno;
            sysworker_dispatch_thread(lseek_handler, filed);
            *have_reply = false;
            break;
        }
        case SYSCALL_EXECVE: {
            const char *path = (char *)seL4_GetMR(1);
            int argc = seL4_GetMR(2);
            char **argv = (char **)malloc(sizeof(char *) * argc);
            for (int i = 0; i < argc; ++i) {
                argv[i] = (char *)seL4_GetMR(i + 3);
                assert(!cross_page((void *)path, (void *)argv[i]));
            }
            char *new_path = setup_projection_space(sender_pcb, (void *)path);
            for (int i = 0; i < argc; ++i)
                argv[i] = argv[i] + (seL4_Word)new_path - (seL4_Word)path;
            seL4_SetMR(0, syscall_execve(sender_pcb, new_path, argc, argv));
            destroy_projection_space((void *)new_path);
            free(argv);
            break;
        }
        case SYSCALL_GETCWD: {
            const char *path = (char *)seL4_GetMR(1);
            int len = seL4_GetMR(2);
            char *new_path = setup_projection_space(sender_pcb, (void *)path);
            strcpy(new_path, sender_pcb->cwd);
            destroy_projection_space((void *)new_path);
            break;
        }
        case SYSCALL_CHDIR: {
            const char *path = (char *)seL4_GetMR(1);
            char *new_path = setup_projection_space(sender_pcb, (void *)path);
            strcpy(sender_pcb->cwd, new_path);
            destroy_projection_space((void *)new_path);
            break;
        }
        case SYSCALL_LS: {
            const char *path = (char *)seL4_GetMR(1);
            char *buf = (char *)seL4_GetMR(2);
            char *new_path = setup_projection_space(sender_pcb, (void *)path);
            char *new_buf = setup_projection_space(sender_pcb, (void *)buf);
            ls(new_path, new_buf);
            destroy_projection_space((void *)new_path);
            destroy_projection_space((void *)new_buf);
            break;
        }
        case SYSCALL_PS: {
            char *buf = (char *)seL4_GetMR(1);
            char *new_buf = setup_projection_space(sender_pcb, (void *)buf);
            syscall_ps(new_buf);
            destroy_projection_space((void *)new_buf);
            break;
        }
        case SYSCALL_KILL: {
            int pid = seL4_GetMR(1), sig = seL4_GetMR(2);
            seL4_SetMR(0, syscall_kill(pid, sig));
            break;
        }
        case SYSCALL_GETPID: {
            seL4_SetMR(0, badge);
            break;
        }
        case SYSCALL_EXIT: {
            syscall_kill(badge, 9);
            break;
        }
        default:
            assert(0);
            *have_reply = false;
    }
}