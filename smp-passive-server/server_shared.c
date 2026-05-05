/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define CORE_0_CH 5
#define CORE_1_CH 6

#define UNUSED __attribute__((unused))

/* Because we deliberately subvert libmicrokit in these examples */
#define INPUT_CAP 1
#define REPLY_CAP 4

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    seL4_Word badge = ch;
    seL4_MessageInfo_t tag UNUSED;
    seL4_MessageInfo_t reply_tag = microkit_msginfo_new(0, 0);
    /* To make this simpler this literally just always replies */
    while (true) {
        microkit_dbg_puts("SHARED_SERVER=>RECV_FROM_");
        microkit_dbg_put32(badge & 0x3f);
        microkit_dbg_puts("\n");
        /* the reason we put the INPUT_CAP here is the call comes from it */
        tag = seL4_ReplyRecv(INPUT_CAP, reply_tag, &badge, REPLY_CAP);
    }

    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");
}

void notified(microkit_channel ch) {}