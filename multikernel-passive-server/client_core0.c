/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

#include "benchmark.h"

#define SERVER_CH 0
#define REMOTE_CORE_1_CH 1
#define REMOTE_CORE_2_CH 2
#define REMOTE_CORE_3_CH 3

#define LOCAL_CONTENDER_CH 11

uintptr_t data_region_vaddr;

static inline uint64_t read_cntvct(void)
{
    uint64_t v;
    asm volatile("isb" ::: "memory");
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}


int rec[100000];

void init(void)
{
    while (__atomic_load_n(boot_release2_lock_addr(), __ATOMIC_SEQ_CST) == 0);
    microkit_dbg_puts("client_core0 start\n");

    microkit_notify(REMOTE_CORE_1_CH);

    for (;;) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
        seL4_Word curr = seL4_GetMR(0);
        // microkit_dbg_puts("client[0]: ");
        // microkit_dbg_put32(curr);
        // microkit_dbg_puts("\n");
#if 1
        if (curr >= (WEAK_SCALING_LOOP - 100)) {
            cycles_t now = read_cntvct() * 50;
            cycles_t start = seL4_GetMR(1);
            for (int i = 0; i < 1000; ++i) {
                asm volatile ("nop");
            }
            puts("\n\n\n\n\n\n\n\nclient 0: ");
            puthex64((now - start)/curr);
            puts("\n");
            break;
        }
#endif
    }
}

void notified(microkit_channel ch) {}
