#include <sel4/sel4.h>
#include <sel4utils/vspace_internal.h>
#include <sel4utils/vspace.h>

#include <stdio.h>

#include "../process.h"

static inline sel4utils_res_t *get_reserve_at_vaddr(sel4utils_alloc_data_t *data) {
    for (sel4utils_res_t *i = data->reservation_head; i != NULL; i = i->next) {
        printf("debug: %p %p %d\n", i->start, i->end, i->malloced);
    }
    while (1);
}

static inline sel4utils_res_t *find_reserve(sel4utils_alloc_data_t *data, uintptr_t vaddr) {
    sel4utils_res_t *current = data->reservation_head;
    while (current != NULL) {
        if (vaddr >= current->start && vaddr < current->end) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void handle_vmfault(void *fault_ep, void *unuse0, void *unuse1) {

    while (1) {
        seL4_Word sender_badge;
        seL4_MessageInfo_t fault_msg = seL4_Recv((seL4_CPtr)fault_ep, &sender_badge);
        PCB *sender_pcb = pid_getproc(sender_badge);
        seL4_Fault_tag_t fault_tag = seL4_MessageInfo_get_label(fault_msg);
        seL4_Fault_t fault = seL4_getFault(fault_msg);
        if (fault_tag == seL4_Fault_VMFault) {
            seL4_Word fault_addr = seL4_Fault_VMFault_get_Addr(fault);
            // fault_addr = 0xbfffffff;
            seL4_Word currip = seL4_Fault_VMFault_get_IP(fault);
            seL4_X86_PageDirectory_GetStatusBits_t status = seL4_X86_PageDirectory_GetStatusBits(vspace_get_root(&sender_pcb->proc.vspace), 0x7000000);
            sel4utils_alloc_data_t *data = get_alloc_data(&sender_pcb->proc.vspace);
            // sel4utils_res_t *res = get_reserve_at_vaddr(data);
            for (int i = 0; i < sender_pcb->proc.v_pos; ++i) {
                sel4utils_res_t *res = reservation_to_res(sender_pcb->proc.vframes[i].reserve);
                printf("debug: %p %p\n", res->start, res->end);
            }
            // sel4utils_res_t *res = reservation_to_res(reservation);
            // printf("get an error: %p %p %d\n", fault_addr, currip, sender_badge);
        }
    }
}