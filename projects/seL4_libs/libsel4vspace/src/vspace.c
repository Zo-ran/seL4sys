/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <vspace/vspace.h>
#include <utils/page.h>
#include <sel4utils/process.h>

void *vspace_new_sized_stack(vspace_t *vspace, size_t n_pages, void *proc) {
    int error = 0;
    uintptr_t stack_bottom = STACK_TOP_VADDR - STACK_SIZE;
    uintptr_t cur = stack_bottom;
    sel4utils_process_t *process = proc;

    for (int i = 0; i < n_pages; ++i) {
        reservation_t reserve = vspace_reserve_range_at(vspace, (void *) cur, PAGE_SIZE_4K, seL4_AllRights, 1);
        assert(reserve.res != NULL);
        error = vspace_new_pages_at_vaddr(vspace, (void *) cur, 1, PAGE_BITS_4K, reserve);
        if (process != NULL) {
            process->vframes[process->v_pos].reserve = reserve;
            process->vframes[process->v_pos].dirty = 0;
            process->vframes[process->v_pos].accessed = 0;
            process->vframes[process->v_pos].swap_filename = NULL;
            process->v_pos += 1;
        }
        assert(error == 0);
        cur += PAGE_SIZE_4K;
    }

    return (void *)(stack_bottom + (n_pages * PAGE_SIZE_4K));
}

void vspace_free_sized_stack(vspace_t *vspace, void *stack_top, size_t n_pages)
{
    if (n_pages > 0) {
        uintptr_t stack_bottom = (uintptr_t) stack_top - (n_pages * PAGE_SIZE_4K);
        vspace_unmap_pages(vspace, (void *) stack_bottom, n_pages,
                           seL4_PageBits, (vka_t *) VSPACE_FREE);
        vspace_free_reservation_by_vaddr(vspace, (void *)(stack_bottom - PAGE_SIZE_4K));
    } else {
        ZF_LOGW("Freeing 0 sized stack");
    }
}

void *vspace_new_ipc_buffer(vspace_t *vspace, seL4_CPtr *page) {
    void *vaddr = (void *) IPC_BUFFER_VADDR;
    reservation_t reserve = vspace_reserve_range_at(vspace, vaddr, IPC_BUFFER_SIZE, seL4_AllRights, 1);
    
    if (reserve.res == NULL) {
        return NULL;
    }

    int error = vspace_new_pages_at_vaddr(vspace, vaddr, BYTES_TO_4K_PAGES(IPC_BUFFER_SIZE), seL4_PageBits, reserve);

    if (error) {
        vspace_free_reservation(vspace, reserve);
        return NULL;
    }

    *page = vspace_get_cap(vspace, vaddr);

    return vaddr;

}

void vspace_free_ipc_buffer(vspace_t *vspace, void *addr)
{
    vspace_unmap_pages(vspace, addr, 1, seL4_PageBits, VSPACE_FREE);
}

void *vspace_share_mem(vspace_t *from, vspace_t *to, void *start, int num_pages, size_t size_bits,
                       seL4_CapRights_t rights, int cacheable)
{
    void *result;

    /* reserve a range to map the shared memory in to */
    reservation_t res = vspace_reserve_range_aligned(to, num_pages * (BIT(size_bits)), size_bits,
                                                     rights, cacheable, &result);

    if (res.res == NULL) {
        ZF_LOGE("Failed to reserve range");
        return NULL;
    }

    int error = vspace_share_mem_at_vaddr(from, to, start, num_pages, size_bits, result, res);
    if (error) {
        /* fail */
        result = NULL;
    }

    /* clean up reservation */
    vspace_free_reservation(to, res);

    return result;
}

int vspace_access_page_with_callback(vspace_t *from, vspace_t *to, void *access_addr, size_t size_bits,
                                     seL4_CapRights_t rights, int cacheable, vspace_access_callback_fn callback, void *cookie)
{
    void *to_vaddr;
    to_vaddr = vspace_share_mem(from, to, access_addr, 1, size_bits, rights, cacheable);
    if (to_vaddr == NULL) {
        ZF_LOGE("Failed to access page");
        return -1;
    }

    /* Invoke callback with mapped address */
    int res = callback(access_addr, to_vaddr, cookie);

    /* Remove mappings from destination vspace */
    vspace_unmap_pages(to, to_vaddr, 1, size_bits, (vka_t *) VSPACE_FREE);

    return res;
}

void *vspace_new_pages_with_config(vspace_t *vspace, vspace_new_pages_config_t *config, seL4_CapRights_t rights)
{
    reservation_t res;
    if (config->vaddr == NULL) {
        res = vspace_reserve_range_aligned(vspace, config->num_pages * SIZE_BITS_TO_BYTES(config->size_bits),
                                           config->size_bits,
                                           rights, true, &config->vaddr);
    } else {
        res =  vspace_reserve_range_at(vspace, config->vaddr,
                                       config->num_pages * SIZE_BITS_TO_BYTES(config->size_bits),
                                       rights, true);
    }
    if (res.res == NULL) {
        ZF_LOGE("Failed to reserve range");
        return NULL;
    }

    UNUSED int error = vspace_new_pages_at_vaddr_with_config(vspace, config, res);
    vspace_free_reservation(vspace, res);

    if (error) {
        return NULL;
    }

    return config->vaddr;
}

/* this function is for backwards compatibility after interface change */
reservation_t vspace_reserve_range(vspace_t *vspace, size_t bytes,
                                   seL4_CapRights_t rights, int cacheable, void **vaddr)
{
    return vspace_reserve_range_aligned(vspace, bytes, seL4_PageBits, rights, cacheable, vaddr);
}
