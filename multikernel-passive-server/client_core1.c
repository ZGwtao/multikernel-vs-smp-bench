/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

#define SERVER_CH 0

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: init function running\n");

    /* message the server */
    int runs = 40;
    while (runs--) {
        microkit_dbg_puts("client 1 - call server on 1\n");
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(1, 1));
    }
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: received a notification on an unexpected channel\n");
}
