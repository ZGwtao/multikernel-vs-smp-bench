/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

#define SERVER_CH 0

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: init function running\n");

    /* message the server */
    int runs = 5;
    while (runs--) {
        microkit_dbg_puts("client 0 - call server on 0\n");
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(1, 1));
    }
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: received a notification on an unexpected channel\n");
}
