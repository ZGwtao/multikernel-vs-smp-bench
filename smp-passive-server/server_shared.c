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
    seL4_MessageInfo_t reply_tag = microkit_msginfo_new(0, 1);

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
        // if (flag) {
        //     seL4_SetMR(0, local_cnt);
        //     seL4_SetMR(1, start);
        //     tag = seL4_ReplyRecv(INPUT_CAP, microkit_msginfo_new(0, 2), &badge, REPLY_CAP);
        //     microkit_dbg_puts("Server: reached 1000000\n");
        //     // for (;;) {}
        // }
        if (local_cnt >= 1000000) {
            // flag = 1;
            seL4_SetMR(0, local_cnt);
            seL4_SetMR(1, start);
            tag = seL4_ReplyRecv(INPUT_CAP, microkit_msginfo_new(0, 2), &badge, REPLY_CAP);
            // microkit_dbg_puts("Server: reached 1000000\n");
        } else {
            seL4_SetMR(0, local_cnt);
            tag = seL4_ReplyRecv(INPUT_CAP, reply_tag, &badge, REPLY_CAP);
        }
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
    // flag = 0;
}

void notified(microkit_channel ch) {}