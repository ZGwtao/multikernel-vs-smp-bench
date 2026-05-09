/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#include "benchmark.h"

cycles_t start;
seL4_Word num;

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    {
        if ((num > 1000000)) {
            for (;;) {}
        }
        if ((start == 0)) {
            start = (seL4_Word)pmu_read_cycles();
        }

        num++;

        if ((num > 1000000)) {
            cycles_t now = pmu_read_cycles();
            cycles_t elapsed = now - start;
            print("cycles per increment: ");
            puthex64(elapsed / num);
            puts("\n");
            for (;;) {}
        }
    }
    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");

    start = 0;
    num = 0;
}

void notified(microkit_channel ch) {}