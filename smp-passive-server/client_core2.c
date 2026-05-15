/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include "benchmark.h"

#define SERVER_CH 10
#define REMOTE_CH 2

#define LOCAL_CONTENDER1_CH 12
#define LOCAL_CONTENDER2_CH 13

static inline uint64_t read_cntvct(void)
{
    uint64_t v;
    asm volatile("isb" ::: "memory");
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}

void init(void)
{
    microkit_dbg_puts("CLIENT_CORE2|INFO: init function running\n");

    pmu_enable();
// }

// void notified(microkit_channel ch)
// {
//     microkit_dbg_puts("CLIENT_CORE2|INFO: received notification from core 0\n");
//     microkit_dbg_puts("client 2: call server\n");
#if 1
    microkit_notify(LOCAL_CONTENDER1_CH);
    microkit_notify(LOCAL_CONTENDER2_CH);
    for (;;) {
#if 0
        for (int i = 0; i < 1000; i++) {
            /* busy wait to increase the chance of preemption */
            asm volatile("nop");
        }
#endif
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
            puthex64((curr_timestamp - start_timestamp)/local_cnt);
            puts("\n");
            for (;;) {}
        }
    }
#else
    microkit_notify(REMOTE_CH);
    (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
#endif
}

void notified(microkit_channel ch) {}