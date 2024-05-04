#pragma once

#include "ioport.h"
#include <allocman/vka.h>

void timer_init(vka_t *vka, vspace_t *vspace, simple_t *simple);
void timer_sleep(seL4_Word microsec);
uint64_t timer_get_time();