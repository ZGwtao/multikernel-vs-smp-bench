/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include "benchmark.h"

#define SERVER_CH 10
#define REMOTE_CH 2

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: init function running\n");

    pmu_enable();
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: received notification from core 0\n");
    microkit_dbg_puts("client 1: call server\n");
#if 1
    microkit_notify(REMOTE_CH);
    for (;;) {
#if 0
        for (int i = 0; i < 1000; i++) {
            /* busy wait to increase the chance of preemption */
            asm volatile("nop");
        }
#endif
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
    }
#else
    microkit_notify(REMOTE_CH);
    (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
#endif
}
