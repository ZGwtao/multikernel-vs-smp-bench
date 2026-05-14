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
#define REMOTE_CH 1

#define LOCAL_CONTENDER_CH 11

uintptr_t data_region_vaddr;

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: init function running\n");
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE1|INFO: received SGI from core 0\n");

    microkit_notify(REMOTE_CH);
    microkit_notify(LOCAL_CONTENDER_CH);

    while (1) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
    }
}
