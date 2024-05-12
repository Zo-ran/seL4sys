#pragma once

typedef void (*bgworker_callback_fn)(void* data);
void sysworker_dispatch_thread(bgworker_callback_fn fn, void *data);
void sysworkers_init();