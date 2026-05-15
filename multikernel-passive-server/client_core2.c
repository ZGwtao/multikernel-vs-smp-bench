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

#define LOCAL_CONTENDER_CH 11

uintptr_t data_region_vaddr;

static inline uint64_t read_cntvct(void)
{
    uint64_t v;
    asm volatile("isb" ::: "memory");
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}


void init(void)
{
    // print("CLIENT_CORE2|INFO: init function running\n");
// }

// void notified(microkit_channel ch)
// {
    // print("CLIENT_CORE2|INFO: received SGI from core 0\n");
    microkit_notify(LOCAL_CONTENDER_CH);
    while (1) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
        seL4_Word curr = seL4_GetMR(0);
        if (curr >= 1000000) {
            cycles_t now = read_cntvct() * 50;
            cycles_t start = seL4_GetMR(1);
            for (int i = 0; i < 150000; ++i) {
                asm volatile ("nop");
            }
            puts("client2: ");
            puthex64((now - start)/curr);
            puts("\n");
            for (;;) {}
        }
    }
}


void notified(microkit_channel ch) {}