/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>
#include <assert.h>

#include "benchmark.h"

#define SERVER_CH 0
#define REMOTE_CORE_1_CH 1
#define REMOTE_CORE_2_CH 2
#define REMOTE_CORE_3_CH 3

#define LOCAL_CONTENDER_CH 11

uintptr_t data_region_vaddr;

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: init function running\n");

    microkit_notify(REMOTE_CORE_1_CH);
    microkit_notify(REMOTE_CORE_2_CH);
    microkit_notify(REMOTE_CORE_3_CH);
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: received SGI from core 1\n");
    microkit_notify(LOCAL_CONTENDER_CH);
    while (1) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
    }
}
