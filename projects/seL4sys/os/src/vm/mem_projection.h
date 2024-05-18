#pragma once

#include "../../include/pcb.h"

void *setup_projection_space(PCB *pcb, void *vaddr);
void destroy_projection_space(void *vaddr);

static inline int cross_page(void *vaddr1, void *vaddr2) {
    return ((seL4_Word)vaddr1 / PAGE_SIZE_4K) != ((seL4_Word)vaddr1 / PAGE_SIZE_4K);
}