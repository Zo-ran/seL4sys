#include <stdio.h>
#include <bits/errno.h>

long _sys_set_tid_address(va_list ap)
{
    return -ENOSYS;
}
