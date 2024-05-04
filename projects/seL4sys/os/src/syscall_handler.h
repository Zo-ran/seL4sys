#pragma once

#include <sel4/sel4.h>

void handle_syscall(seL4_MessageInfo_t msg_tag, bool *have_reply, seL4_MessageInfo_t *reply_tag, seL4_Word badge);