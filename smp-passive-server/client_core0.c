/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

#include "benchmark.h"

#define SERVER_CH 10
#define REMOTE_CORE_1_CH 2
#define REMOTE_CORE_2_CH 3
#define REMOTE_CORE_3_CH 4

#define LOCAL_CONTENDER_CH 12

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: init function running\n");
    microkit_dbg_puts("CLIENT_CORE0|INFO: done initial call to server, now notifying core 1\n");

    pmu_enable();

    microkit_notify(REMOTE_CORE_1_CH);
    microkit_notify(REMOTE_CORE_2_CH);
    microkit_notify(REMOTE_CORE_3_CH);
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: received notification from core 1\n");
    microkit_dbg_puts("client 0 - call server on 0\n");
#if 1
    microkit_notify(LOCAL_CONTENDER_CH);
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
