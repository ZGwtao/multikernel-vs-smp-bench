/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include "benchmark.h"

#define SERVER_CH 0
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
        seL4_Word curr = seL4_GetMR(0);
        if (curr >= 1000000) {
            cycles_t now = read_cntvct() * 50;
            cycles_t start = seL4_GetMR(1);
            for (int i = 0; i < 100 * (curr / 3000); ++i) {
                asm volatile ("nop");
            }
            puts("\n");
            puthex64((now - start)/curr);
            puts("\n");
            for (;;) {}
        }
    }
}
