#include "mem_projection.h"
#include "../rootvars.h"
#include <sel4utils/mapping.h>
#include <utils/page.h>

void *setup_projection_space(PCB *pcb, void *vaddr) {
    seL4_Word page_addr = PAGE_ALIGN_4K((seL4_Word)vaddr);
    seL4_CPtr frame = vspace_get_cap(&pcb->proc.vspace, (void *)page_addr);
    void *addr = sel4utils_dup_and_map(&vka, &vspace, frame, PAGE_BITS_4K);
    return addr + (seL4_Word)vaddr - page_addr;
}

void destroy_projection_space(void *vaddr) {
    seL4_Word page_addr = PAGE_ALIGN_4K((seL4_Word)vaddr);
    sel4utils_unmap_dup(&vka, &vspace, (void *)page_addr, PAGE_BITS_4K);
}