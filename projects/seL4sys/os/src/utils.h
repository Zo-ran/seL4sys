#pragma once


#define FUNC_IFERR(msg, func, ...) ZF_LOGF_IFERR(func(__VA_ARGS__), msg);