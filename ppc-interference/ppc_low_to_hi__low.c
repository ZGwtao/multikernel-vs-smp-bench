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

#define CONFIG_BREAK_DOWN_ANALYSIS 1
#undef CONFIG_BREAK_DOWN_ANALYSIS

#ifdef CONFIG_BREAK_DOWN_ANALYSIS

#define BIT(n) (1 << (n))

#include <sel4/config.h>
#include <sel4/benchmark_tracepoints_types.h>


#define TRACE_NUM_PER_ROUNDTRIP (2)
#define ROUNDTRIP_PER_SAMPLE (100)
#define TRACE_NUM_PER_SAMPLE (TRACE_NUM_PER_ROUNDTRIP * ROUNDTRIP_PER_SAMPLE)


#if CONFIG_MAX_NUM_TRACE_POINTS > 0
#define KERNEL_MAX_NUM_LOG_ENTRIES (BIT(seL4_LargePageBits) / sizeof(benchmark_tracepoint_log_entry_t))
typedef benchmark_tracepoint_log_entry_t  kernel_log_entry_t;
#else
#define KERNEL_MAX_NUM_LOG_ENTRIES 0
typedef void *kernel_log_entry_t;
#endif

/* Copies up to n entries from the kernel's internal log to the specified array,
 * returning the number of entries copied.
 */
unsigned int kernel_logging_sync_log(kernel_log_entry_t log[], unsigned int n);

/* Returns the key field of a log entry. */
static inline seL4_Word kernel_logging_entry_get_key(kernel_log_entry_t *entry)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return entry->id;
#else
    return 0;
#endif
}

/* Sets the key field of a log entry to a given value. */
static inline void kernel_logging_entry_set_key(kernel_log_entry_t *entry, seL4_Word key)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    entry->id = key;
#endif
}

/* Returns the data field of a log entry. */
static inline seL4_Word kernel_logging_entry_get_data(kernel_log_entry_t *entry)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    return entry->duration;
#else
    return 0;
#endif
}

/* Sets the data field of a log entry to a given value. */
static inline void kernel_logging_entry_set_data(kernel_log_entry_t *entry, seL4_Word data)
{
#if CONFIG_MAX_NUM_TRACE_POINTS > 0
    entry->duration = data;
#endif
}

/* Resets the log buffer to contain no entries. */
static inline void kernel_logging_reset_log(void)
{
#ifdef CONFIG_ENABLE_BENCHMARKS
    seL4_BenchmarkResetLog();
#endif /* CONFIG_ENABLE_BENCHMARKS */
}

/* Calls to kernel_logging_sync_log will extract entries created before
 * the most-recent call to this function. Call this function before calling
 * kernel_logging_sync_log. */
static inline void kernel_logging_finalize_log(void)
{
#ifdef CONFIG_ENABLE_BENCHMARKS
    seL4_BenchmarkFinalizeLog();
#endif /* CONFIG_ENABLE_BENCHMARKS */
}

/* Tell the kernel about the allocated user-level buffer
 * so that it can write to it. Note, this function has to
 * be called before kernel_logging_reset_log.
 *
 * @logBuffer_cap should be a cap of a large frame size.
 */
static inline seL4_Error kernel_logging_set_log_buffer(seL4_CPtr logBuffer_cap)
{
#ifdef CONFIG_KERNEL_LOG_BUFFER
    return seL4_BenchmarkSetLogBuffer(logBuffer_cap);
#else
    return seL4_NoError;
#endif /* CONFIG_KERNEL_LOG_BUFFER */
}

static inline void kernel_logging_dump_full_log(kernel_log_entry_t *logBuffer, size_t logSize)
{
    seL4_Word index = 0;

    while ((index * sizeof(kernel_log_entry_t)) < logSize) {
        if (logBuffer[index].duration != 0) {
            print("tracepoint id = ");
            puthex64(logBuffer[index].id);
            puts(" \tduration = ");
            puthex64(logBuffer[index].duration);
            puts("\n");
        }
        index++;
    }

    print("Dumped entire log, size ");
    puthex64(index);
    puts("\n");
}
#endif /* CONFIG_BREAK_DOWN_ANALYSIS */

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

    for (size_t i = 0; i < NUM_SAMPLES; i++) {

        start = pmu_read_cycles();
        /* ==== Benchmark critical ==== */
        {
            /* Call high (does not switch threads) */
            seL4_Call(BASE_ENDPOINT_CAP + PPC_HI_LO_CHANNEL, microkit_msginfo_new(0, 0));
        }
        end = pmu_read_cycles();
        RECORDING_ADD_SAMPLE(start, end);
    }

    RECORDING_END(results);
#ifdef CONFIG_BREAK_DOWN_ANALYSIS
    {
        seL4_Word err = kernel_logging_set_log_buffer(8);
        if (err != seL4_NoError) {
            print("failed to set log buffer\n");
            microkit_internal_crash(err);
        }
        kernel_logging_reset_log();
        for (size_t i = 0; i < ROUNDTRIP_PER_SAMPLE; i++) {
            /* ==== Benchmark critical ==== */
            {
                /* Call high (does not switch threads) */
                seL4_Call(BASE_ENDPOINT_CAP + PPC_HI_LO_CHANNEL, microkit_msginfo_new(0, 0));
            }
        }
        kernel_logging_finalize_log();
        kernel_logging_dump_full_log((kernel_log_entry_t *)0xffc00000, TRACE_NUM_PER_SAMPLE * sizeof(kernel_log_entry_t));
    }
#endif
    microkit_notify(BENCHMARK_START_STOP_CH);
}

DECLARE_SUBVERTED_MICROKIT()
