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

uintptr_t data_region_vaddr;

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: init function running\n");

    microkit_notify(REMOTE_CH);
}

void notified(microkit_channel ch)
{
    microkit_dbg_puts("CLIENT_CORE0|INFO: received SGI from core 1\n");
    while (1) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(1, 1));
    }
}
