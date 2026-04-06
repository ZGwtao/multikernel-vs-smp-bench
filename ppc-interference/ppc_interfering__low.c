/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>
#include <assert.h>

#include "benchmark.h"
#include "config.h"

#define PPC_HI_LO_CHANNEL 2

uintptr_t results;

void init(void)
{
    print("interferring client\n");
    for (;;) {
        {
            /* Call high (does not switch threads) */
            seL4_Call(BASE_ENDPOINT_CAP + PPC_HI_LO_CHANNEL, microkit_msginfo_new(0, 0));
        }
    }
}

DECLARE_SUBVERTED_MICROKIT()
