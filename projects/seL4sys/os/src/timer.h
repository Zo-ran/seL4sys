#pragma once

#include "rootvars.h"

void timer_init(void);
void timer_sleep(seL4_Word microsec);
uint64_t timer_get_time();