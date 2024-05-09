#include <sel4/sel4.h>
#include <sel4utils/vspace_internal.h>
#include <sel4utils/vspace.h>

#include <stdio.h>

#include "../process.h"
#include "../rootvars.h"
#include "../filesystem/pagefile.h"

static inline VFrame *find_vframe(VFrame *head, seL4_Word vaddr) {
    for (VFrame *curr = head; curr != NULL; curr = curr->next) {
        sel4utils_res_t *res = reservation_to_res(curr->reserve);
        if (vaddr >= res->start && vaddr < res->end)
            return curr;
    }
    return NULL;
}

static inline void page_replace(PCB *pcb, VFrame *vframe) {
    printf("page replace begin\n");
    // update dirty and accessed information
    for (VFrame *curr = pcb->proc.vframes; curr != NULL; curr = curr->next)
        if (curr->inRAM) {
            sel4utils_res_t *res = reservation_to_res(curr->reserve);
            seL4_X86_PageDirectory_GetStatusBits_t status = seL4_X86_PageDirectory_GetStatusBits(pcb->proc.pd.cptr, res->start);
            curr->accessed = status.accessed;
            curr->dirty = status.dirty;
            printf("addr: %p dirty: %d accessed: %d\n", res->start, curr->dirty, curr->accessed);
        }

    VFrame *replaced_page = NULL;
    // find a page to replace (second chance algorithm)
    while (replaced_page == NULL) {
        if (pcb->proc.replacer->inRAM) {
            VFrame *replacer = pcb->proc.replacer;
            // printf("debug: %d %d\n", replacer->accessed, replacer->dirty);
            if (!replacer->accessed && !replacer->dirty) {
                replaced_page = pcb->proc.replacer;
                break;
            } else if (replacer->dirty) {
                replacer->dirty = 0;
            } else {
                replacer->accessed = 0;
            }
        }
        pcb->proc.replacer = pcb->proc.replacer->next != NULL ? pcb->proc.replacer->next : pcb->proc.vframes;
    }
    sel4utils_res_t *res = reservation_to_res(replaced_page->reserve);
    seL4_CPtr frame = vspace_get_cap(&pcb->proc.vspace, (void *)res->start);
    void *mapping = sel4utils_dup_and_map(&vka, &vspace, frame, PAGE_BITS_4K);
    // TODO: 将要替换下去的页写入文件
    if (replaced_page->pagefile == NULL) {
        replaced_page->pagefile = get_pagefile();
        printf("pfname: %s\n", replaced_page->pagefile);
    }
    printf("replaced: %p %p\n", res->start, res->end);
}

static inline void alloc_frame(PCB *pcb, VFrame *vframe) {
    if (pcb->proc.pframes_used >= MAX_PFRAME_NUM) {
        // need to replace a page
        page_replace(pcb, vframe);
    } else {
        pcb->proc.pframes_used += 1;
        sel4utils_res_t *res = reservation_to_res(vframe->reserve);
        vspace_new_pages_at_vaddr(&pcb->proc.vspace, (void *)res->start, 1, PAGE_BITS_4K, vframe->reserve);
        vframe->inRAM = 1;
    }
}

void handle_vmfault(seL4_MessageInfo_t fault_msg, bool *resume, seL4_Word sender_badge) {
    PCB *sender_pcb = pid_getproc(sender_badge);
    seL4_Fault_tag_t fault_tag = seL4_MessageInfo_get_label(fault_msg);
    seL4_Fault_t fault = seL4_getFault(fault_msg);
    if (fault_tag == seL4_Fault_VMFault) {
        seL4_Word fault_vaddr = seL4_Fault_VMFault_get_Addr(fault);
        seL4_Word currip = seL4_Fault_VMFault_get_IP(fault);
        VFrame *vframe = find_vframe(sender_pcb->proc.vframes, fault_vaddr);

        if (fault_vaddr >= sender_pcb->heap_top && fault_vaddr < STACK_TOP_VADDR) {
            // need to expand stack

        } else {
            if (vframe == NULL) {
                ZF_LOGE("VM fault at vaddr: %p (IP %p).", fault_vaddr, currip);
                *resume = false;
            } else {
                alloc_frame(sender_pcb, vframe);
                *resume = false;
            }
        }
    }
}

void handle_vmfault_loop(void *fault_ep, void *unuse0, void *unuse1) {
    bool resume = false;
    seL4_Word sender_badge;
    seL4_MessageInfo_t fault_msg, reply_msg;
    while (1) {
        if (resume) {
            fault_msg = seL4_ReplyRecv((seL4_CPtr)fault_ep, reply_msg, &sender_badge);
        } else {
            fault_msg = seL4_Recv((seL4_CPtr)fault_ep, &sender_badge);
        }
        handle_vmfault(fault_msg, &resume, sender_badge);
    }
}