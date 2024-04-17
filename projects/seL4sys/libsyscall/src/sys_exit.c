#include <stdio.h>
#include <stdlib.h>
#include <sel4/sel4.h>
#include <sos.h>

static void sosapi_abort()
{
    sos_process_delete(sos_my_id());
    while (1); /* We don't return after this */
}

long _sys_rt_sigprocmask(va_list ap)
{
    /* abort messages with signals in order to kill itself */
    return 0;
}

long _sys_gettid(va_list ap)
{
    /* return dummy for now */
    return 0;
}

long _sys_getpid(va_list ap)
{
    /* assuming process IDs are the same as thread IDs*/
    return 0;
}

long _sys_exit(va_list ap)
{
    sosapi_abort();
    return 0;
}

long _sys_exit_group(va_list ap)
{
    sosapi_abort();
    return 0;
}

long _sys_tgkill(va_list ap)
{
    sosapi_abort();
    return 0;
}

