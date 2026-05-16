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
    microkit_dbg_puts("CLIENT_CORE0|INFO: init function running\n");
    microkit_dbg_puts("CLIENT_CORE0|INFO: done initial call to server, now notifying core 1\n");

    pmu_enable();

    microkit_notify(LOCAL_CONTENDER1_CH);
    microkit_notify(LOCAL_CONTENDER2_CH);

    for (int i = 0; i < WEAK_SCALING_BATCH; ++i) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
#if 0
        seL4_Word local_cnt = seL4_GetMR(0);
        if (local_cnt >= 1000000) {
            cycles_t curr_timestamp = read_cntvct() * 50;
            cycles_t start_timestamp = seL4_GetMR(1);
            for (int i = 0; i < 10000; ++i) {
                asm volatile ("nop");
            }
            puthex64((curr_timestamp - start_timestamp)/local_cnt);
            puts("\n");
            for (;;) {}
        }
#endif
    }
}

void notified(microkit_channel ch) {}
