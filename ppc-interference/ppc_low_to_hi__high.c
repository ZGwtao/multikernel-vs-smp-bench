/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>

#include "benchmark.h"

#define PPC_HI_LO_CHANNEL 2

void init(void)
{
    print("hello world\n");
#if 1
    {
        seL4_TCB_SetFlags_t res;
        seL4_TCBFlag flags_clear = seL4_TCBFlag_NoFlag, flags_set = seL4_TCBFlag_fpuDisabled;

        res = seL4_TCB_SetFlags(6, flags_clear, flags_set);
    }
#endif
}

DECLARE_SUBVERTED_MICROKIT()

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    seL4_Word badge;
    seL4_MessageInfo_t tag UNUSED;
    seL4_MessageInfo_t reply_tag;
    /* To make this simpler this literally just always replies */
    while (true) {
        /* the reason we put the INPUT_CAP here is the call comes from it */
        tag = seL4_ReplyRecv(INPUT_CAP, reply_tag, &badge, REPLY_CAP);
    }
    return seL4_MessageInfo_new(0, 0, 0, 0);
}
