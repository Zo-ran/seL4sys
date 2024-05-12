#pragma once

#include <vspace/vspace.h>
#include <simple/simple.h>
#include <sel4utils/thread.h>

void start_system_thread(const char *name, int priority, sel4utils_thread_entry_fn entry_point, void *arg0, void *arg1, void *arg2);

extern vspace_t vspace;
extern vka_t vka;
extern vka_object_t fault_ep;
extern seL4_CPtr syscall_ep;
extern simple_t simple;