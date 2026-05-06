/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

#define SERVER_CH 10
#define REMOTE_CH 2

#define INPUT_CAP 1
#define REPLY_CAP 4

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: init function running\n");
    microkit_dbg_puts("CLIENT_CORE0|INFO: done initial call to server, now notifying core 1\n");

    microkit_notify(REMOTE_CH);
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: received notification from core 1\n");
    microkit_dbg_puts("client 0 - call server on 0\n");
#if 1
    for (;;) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
    }
#else
    (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
    microkit_notify(REMOTE_CH);
#endif
}
