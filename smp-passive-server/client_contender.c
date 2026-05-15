/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include "benchmark.h"

#define SERVER_CH 10
#define REMOTE_CH 2

static inline uint64_t read_cntvct(void)
{
    uint64_t v;
    asm volatile("isb" ::: "memory");
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}

void init(void) {}

void notified(microkit_channel ch)
{
    for (;;) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
        seL4_Word local_cnt = seL4_GetMR(0);
        if (local_cnt >= 1000000) {
            cycles_t curr_timestamp = read_cntvct() * 50;
            cycles_t start_timestamp = seL4_GetMR(1);
            // microkit_dbg_puts("\n");
            // microkit_dbg_puts("Server reached 1000000, eventually\n");
            // microkit_dbg_puts("Total:");
            // microkit_dbg_put32(curr_timestamp - start_timestamp);
            // microkit_dbg_puts("\n");
            // microkit_dbg_puts("Average:");
            for (int i = 0; i < 10000; ++i) {
                asm volatile ("nop");
            }
            microkit_dbg_put32((curr_timestamp - start_timestamp)/local_cnt);
            microkit_dbg_puts("\n");
            for (;;) {}
        }
    }
}
