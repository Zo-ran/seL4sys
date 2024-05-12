#include <string.h>
#include <assert.h>
#include <vm_layout.h>

size_t first_empty_pos;

static inline void puts_shared_data(const void *data, size_t len) {
    assert(first_empty_pos + len < SYS_SHARED_AREA_SIZE);
    memcpy(first_empty_pos + (void *)SYS_SHARED_AREA_VADDR, data, len);
    first_empty_pos += len;
}

void *puts_shared_str(const char *str) {
    size_t ret = first_empty_pos;
    puts_shared_data(str, strlen(str) + 1);
    return ret + (void *)SYS_SHARED_AREA_VADDR;
}

void *get_shared_area(size_t len) {
    size_t ret = first_empty_pos;
    assert(first_empty_pos + len < SYS_SHARED_AREA_SIZE);
    first_empty_pos += len;
    return ret + (void *)SYS_SHARED_AREA_VADDR;
}