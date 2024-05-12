#include <sel4/sel4.h>
#include <sel4utils/vspace_internal.h>
#include <sel4utils/vspace.h>
#include <vka/capops.h>

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

static inline void unmap_page(seL4_CPtr frame) {
    seL4_ARCH_Page_Unmap(frame);
    cspacepath_t path;
    vka_cspace_make_path(&vka, frame, &path);
    vka_cnode_delete(&path);
    vka_cspace_free(&vka, frame);
}

static inline void page_replace(PCB *pcb, VFrame *vframe) {
    // update dirty and accessed information
    for (VFrame *curr = pcb->proc.vframes; curr != NULL; curr = curr->next)
        if (curr->inRAM) {
            sel4utils_res_t *res = reservation_to_res(curr->reserve);
            seL4_X86_PageDirectory_GetStatusBits_t status = seL4_X86_PageDirectory_GetStatusBits(pcb->proc.pd.cptr, res->start);
            curr->accessed = status.accessed * 2;
            curr->dirty = status.dirty * 2;
        }

    VFrame *replaced_page = NULL;
    // find a page to replace (second chance algorithm)
    while (replaced_page == NULL) {
        if (pcb->proc.replacer->inRAM) {
            VFrame *replacer = pcb->proc.replacer;
            if (replacer->accessed < 2 && replacer->dirty < 2) {
                replaced_page = pcb->proc.replacer;
                break;
            } else if (replacer->dirty == 2) {
                replacer->dirty = 1;
            } else {
                replacer->accessed = 1;
            }
        }
        pcb->proc.replacer = pcb->proc.replacer->next != NULL ? pcb->proc.replacer->next : pcb->proc.vframes;
    }
    // no linked pagefile, get one
    seL4_Word save = replaced_page->dirty;
    if (replaced_page->pagefile == NULL) {
        replaced_page->pagefile = get_pagefile();
        save = 1;
    }

    sel4utils_res_t *res = reservation_to_res(replaced_page->reserve);
    seL4_CPtr frame = vspace_get_cap(&pcb->proc.vspace, (void *)res->start);

    // if modified or not saved ever, save to page file 
    if (save) {
        void *mapping = sel4utils_dup_and_map(&vka, &vspace, frame, PAGE_BITS_4K);
        write_page_to_file(replaced_page->pagefile, mapping);
        sel4utils_unmap_dup(&vka, &vspace, mapping, PAGE_BITS_4K);
    }

    // duplicate the frame, then unmap saved page
    seL4_CPtr tmp_frame;
    vka_cspace_alloc(&vka, &tmp_frame);
    seL4_CNode_Copy(
        seL4_CapInitThreadCNode, tmp_frame, seL4_WordBits, 
        seL4_CapInitThreadCNode, frame, seL4_WordBits,
        seL4_AllRights
    );
    unmap_page(frame);
    reserve_entries(&pcb->proc.vspace, res->start, PAGE_BITS_4K);
    replaced_page->inRAM = 0;

    // read pagefile to target page
    if (vframe->pagefile != NULL) {
        void *mapping = sel4utils_dup_and_map(&vka, &vspace, tmp_frame, PAGE_BITS_4K);
        read_file_to_page(vframe->pagefile, mapping);
        sel4utils_unmap_dup(&vka, &vspace, mapping, PAGE_BITS_4K);
    }

    // map target page
    res = reservation_to_res(vframe->reserve);
    vspace_map_pages_at_vaddr(&pcb->proc.vspace, &tmp_frame, NULL, (void *)res->start, 1, PAGE_BITS_4K, vframe->reserve);
    vframe->inRAM = 1;
}

static inline void alloc_frame(PCB *pcb, VFrame *vframe) {
    if (pcb->proc.pframes_used >= MAX_PFRAME_NUM) {
        page_replace(pcb, vframe);
    } else {
        pcb->proc.pframes_used += 1;
        sel4utils_res_t *res = reservation_to_res(vframe->reserve);
        vspace_new_pages_at_vaddr(&pcb->proc.vspace, (void *)res->start, 1, PAGE_BITS_4K, vframe->reserve);
        vframe->inRAM = 1;
    }
}

void expand_stack(PCB *pcb, seL4_Word fault_vaddr) {
    seL4_Word vaddr = PAGE_ALIGN_4K(fault_vaddr);
    reservation_t reserve = vspace_reserve_range_at(&pcb->proc.vspace, (void *) vaddr, PAGE_SIZE_4K, seL4_AllRights, 1);
    VFrame *vframe = (VFrame *)malloc(sizeof(VFrame));
    vframe->reserve = reserve;
    vframe->dirty = 0;
    vframe->accessed = 0;
    vframe->pagefile = NULL;
    vframe->inRAM = 0;
    vframe->next = pcb->proc.vframes;
    pcb->proc.vframes = vframe;
    alloc_frame(pcb, vframe);
}

void handle_vmfault(seL4_MessageInfo_t fault_msg, bool *resume, seL4_Word sender_badge) {
    PCB *sender_pcb = pid_getproc(sender_badge);
    seL4_Fault_tag_t fault_tag = seL4_MessageInfo_get_label(fault_msg);
    seL4_Fault_t fault = seL4_getFault(fault_msg);
    if (fault_tag == seL4_Fault_VMFault) {
        seL4_Word fault_vaddr = seL4_Fault_VMFault_get_Addr(fault);
        seL4_Word currip = seL4_Fault_VMFault_get_IP(fault);
        VFrame *vframe = find_vframe(sender_pcb->proc.vframes, fault_vaddr);
        if (fault_vaddr >= sender_pcb->heap_top && fault_vaddr >= HEAP_BASE_VADDR && fault_vaddr < STACK_TOP_VADDR) {
            // need to expand stack
            expand_stack(sender_pcb, fault_vaddr);
            *resume = true;
        } else {
            if (vframe == NULL) {
                // need to destroy the processw
                if (currip == 0)
                    syscall_kill(sender_badge, 9);
                else
                    ZF_LOGE("VM fault at vaddr: %p (IP %p).", fault_vaddr, currip);
                *resume = false;
            } else {
                alloc_frame(sender_pcb, vframe);
                *resume = true;
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