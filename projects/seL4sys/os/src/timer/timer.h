#pragma once

#include "../ioport.h"
#include <allocman/vka.h>

typedef struct TimeData {
    seL4_CPtr reply;
    seL4_Word time;
} TimeData;


void timer_init(vka_t *vka, vspace_t *vspace, simple_t *simple);
void timer_sleep(void *data);
uint64_t timer_get_time();