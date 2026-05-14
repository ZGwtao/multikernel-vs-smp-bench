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

cycles_t curr;
cycles_t last;
cycles_t local_cnt;
cycles_t gap;

seL4_Word flag;

static inline uint64_t read_cntvct(void)
{
    uint64_t v;
    asm volatile("isb" ::: "memory");
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}

static inline uint64_t read_cntfrq(void)
{
    uint64_t v;
    asm volatile("mrs %0, cntfrq_el0" : "=r"(v));
    return v;
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    RECORDING_BEGIN();

    seL4_Word badge;
    seL4_MessageInfo_t tag UNUSED;
    seL4_MessageInfo_t reply_tag = microkit_msginfo_new(0, 0);

    cycles_t start = 0;

    while (1) {
        curr = read_cntvct() * 50;

        if (last != 0) {
            gap = curr - last;

            if (gap != 0) {
                RECORDING_ADD_SAMPLE(last, curr)
                local_cnt++;
            }
        } else {
            start = curr;
        }

        last = curr;

        if (flag) {
            for (;;) {}
        }

        if (local_cnt >= 1000000) {
            flag = 1;

            cycles_t now = read_cntvct() * 50;
            cycles_t elapsed = now - start;
            
            print("global time elapsed in cycles: ");
            puthex64(elapsed);
            puts("\n");

            print("average cycles per increment: ");
            puthex64(elapsed / local_cnt);
            puts("\n");

            print("CNTFRQ_EL0: ");
            puthex64(read_cntfrq());
            puts("\n");

            print("sum cntvct ticks in cycles recorded: ");
            puthex64(sum);
            puts("\n");

            print("average cntvct ticks in cycles recorded: ");
            puthex64(sum / local_cnt);
            puts("\n");

            print("local sample count: ");
            puthex64(local_cnt);
            puts("\n");

            print("sum squared cntvct ticks in cycles recorded: ");
            puthex64(sum_squared);
            puts("\n");

            print("min cntvct ticks in cycles recorded: ");
            puthex64(min);
            puts("\n");

            print("max cntvct ticks in cycles recorded: ");
            puthex64(max);
            puts("\n");

            for (;;) {}
        }

        tag = seL4_ReplyRecv(INPUT_CAP, reply_tag, &badge, REPLY_CAP);
    }

    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");

    local_cnt = 0;
    last = 0;
    gap = 0;
    curr = 0;
    flag = 0;
}

void notified(microkit_channel ch) {}