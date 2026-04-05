/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>
#include <assert.h>

#include "benchmark.h"
#include "config.h"

#define PPC_HI_LO_CHANNEL 2

uintptr_t results;

void init(void)
{
    seL4_Word badge;
    seL4_MessageInfo_t tag UNUSED;
    cycles_t start;
    cycles_t end;

    print("hello world\n");

    /* wait for start notification */
    tag = seL4_Recv(INPUT_CAP, &badge, REPLY_CAP);

    RECORDING_BEGIN();

    for (size_t i = 0; i < NUM_WARMUP; i++) {
        start = pmu_read_cycles();
        seL4_Call(BASE_ENDPOINT_CAP + PPC_HI_LO_CHANNEL, microkit_msginfo_new(0, 0));
        end = pmu_read_cycles();

        asm volatile("" :: "r"(start), "r"(end));
    }

    start = pmu_read_cycles();
    for (size_t i = 0; i < NUM_SAMPLES; i++) {

        /* ==== Benchmark critical ==== */
        {
            /* Call high (does not switch threads) */
            seL4_Call(BASE_ENDPOINT_CAP + PPC_HI_LO_CHANNEL, microkit_msginfo_new(0, 0));
        }

    }
    end = pmu_read_cycles();
    RECORDING_ADD_SAMPLE(start, end);

    RECORDING_END(results);

    microkit_notify(BENCHMARK_START_STOP_CH);
}

DECLARE_SUBVERTED_MICROKIT()
