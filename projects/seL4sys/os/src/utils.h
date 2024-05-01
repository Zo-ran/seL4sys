#pragma once

// #define FUNC_IFERR(msg, func, ...) ZF_LOGF_IFERR(func(__VA_ARGS__), msg);

#include <arch_stdio.h>

#define test_printf(msg, ...) \
    char buf[64] = "\0";\
    sprintf(buf, msg, __VA_ARGS__); \
    __arch_write(buf, strlen(buf))

#define FUNC_IFERR(msg, func, ...) \
    if (func(__VA_ARGS__)) \
        for (int i = 0; i < strlen(msg); ++i) \
            __arch_putchar(msg[i]);

;