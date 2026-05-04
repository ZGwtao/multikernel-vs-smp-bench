/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdint.h>
#include <stddef.h>

#define CORE_0_CH 5
#define CORE_1_CH 6


microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{

    switch (microkit_msginfo_get_label(msginfo)) {
    case 1:
        microkit_dbg_puts("SERVER_SHARED: received a ppc call from channel ");
        if (ch == CORE_0_CH) {
            microkit_dbg_puts("0\n");
        } else if (ch == CORE_1_CH) {
            microkit_dbg_puts("1\n");
        } else {
            microkit_dbg_puts("unknown\n");
        }
        break;
    default:
        microkit_dbg_puts("SERVER_SHARED|ERROR: received an unexpected message\n");
    }

    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");
}

void notified(microkit_channel ch) {}