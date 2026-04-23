/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    switch (microkit_msginfo_get_label(msginfo)) {
    case 1:
        microkit_dbg_puts("SERVER_SHARED: received a ppc call\n");
        break;
    default:
        microkit_dbg_puts("SERVER_SHARED|ERROR: received an unexpected message\n");
    }

    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");
    /* Nothing to initialise */
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("SERVER_SHARED|ERROR: received a notification on an unexpected channel\n");
}
