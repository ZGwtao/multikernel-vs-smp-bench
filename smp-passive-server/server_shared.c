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

cycles_t curr;
cycles_t last;
cycles_t local_cnt;
cycles_t gap;

seL4_Word core_id;
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
    /* To make this simpler this literally just always replies */
    while (1) {
        curr = read_cntvct();
        if (last != 0) {
            gap = curr - last;
        }
        if (gap != 0) {
            RECORDING_ADD_SAMPLE(last, curr)
        }
        last = curr;
        if (flag) {
            for (;;) {}
        }
        if ((start == 0)) {
            start = (seL4_Word)pmu_read_cycles();
            core_id = (ch % 10);
        }

        num++;
        local_cnt++;

        if ((num > 1000000)) {
            if (core_id == (ch % 10)) {
                flag = 1;
                cycles_t now = pmu_read_cycles();
                cycles_t elapsed = now - start;
                print("cycles from pmu per respond: ");
                puthex64(elapsed / num);
                puts("\n");
                uint64_t freq = read_cntfrq();
                print("CNTFRQ_EL0 = ");
                puthex64(freq);
                puts("\n");
                print("sum cycles from cntvct recorded: ");
                puthex64(sum);
                puts("\n");
                print("average cycles from cntvct recorded: ");
                puthex64(sum / local_cnt);
                puts("\n");
                print("local count: ");
                puthex64(local_cnt);
                puts("\n");
                print("sum squared cycles from cntvct recorded: ");
                puthex64(sum_squared);
                puts("\n");
                print("min cycles from cntvct recorded: ");
                puthex64(min);
                puts("\n");
                print("max cycles from cntvct recorded: ");
                puthex64(max);
                puts("\n");
                {
                    uint64_t sum_cntvct_empty = 0;
                    uint64_t sum_pmu_empty = 0;

                    for (uint64_t i = 0; i < 1000000; i++) {
                        uint64_t t0 = read_cntvct();
                        uint64_t t1 = read_cntvct();
                        sum_cntvct_empty += t1 - t0;

                        uint64_t c0 = pmu_read_cycles();
                        uint64_t c1 = pmu_read_cycles();
                        sum_pmu_empty += c1 - c0;
                    }

                    print("sum empty cntvct delta = ");
                    puthex64(sum_cntvct_empty);
                    puts("\n");

                    print("sum empty pmu delta = ");
                    puthex64(sum_pmu_empty);
                    puts("\n");
                }     
                for (;;) {}
            }
        }
        tag = seL4_ReplyRecv(INPUT_CAP, reply_tag, &badge, REPLY_CAP);
    }
    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");

    start = 0;
    num = 0;

    local_cnt = 0;
    last = 0;
    gap = 0;
    curr = 0;
    core_id = 100;
    flag = 0;
}

void notified(microkit_channel ch) {}