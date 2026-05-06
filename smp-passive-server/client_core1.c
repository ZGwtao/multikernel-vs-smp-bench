/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

#define SERVER_CH 10
#define REMOTE_CH 2

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: init function running\n");
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: received notification from core 0\n");
    microkit_dbg_puts("client 1: call server\n");
#if 1
    microkit_notify(REMOTE_CH);
    for (;;) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
    }
#else
    microkit_notify(REMOTE_CH);
    (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
#endif
}
