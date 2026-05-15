/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

#include "benchmark.h"

#define CORE_0_CH 5
#define CORE_1_CH 6

uintptr_t shared_region_vaddr;
uintptr_t data_region_vaddr;

#define SHARED_TIME_STAMP_OFFSET 8UL

static inline seL4_Word *shared_data_region_addr(seL4_Word index)
{
    return (seL4_Word *)(data_region_vaddr + index);
}

static inline seL4_Word *shared_time_stamp_addr(void)
{
    return (seL4_Word *)(data_region_vaddr + SHARED_TIME_STAMP_OFFSET);
}

#define SAMPLE_OFFSET 16UL
#define SUM_OFFSET 17UL
#define SUM_SQUARED_OFFSET 18UL
#define MIN_OFFSET 19UL
#define MAX_OFFSET 20UL
#define LAST_START_OFFSET 21UL
#define LAST_END_OFFSET 22UL

static inline seL4_Word read_shared_data(seL4_Word index)
{
    return *shared_data_region_addr(index);
}

static inline seL4_Word write_shared_data(seL4_Word index, seL4_Word value)
{
    *shared_data_region_addr(index) = value;
    return value;
}

/*
 * Use the first 8 bytes of the shared region as the lock word.
 *
 * 0 = unlocked
 * 1 = locked
 */
#define SHARED_LOCK_OFFSET      0UL
#define SHARED_LOCK_UNLOCKED    0UL
#define SHARED_LOCK_LOCKED      1UL

static inline volatile uint64_t *
shared_lock_addr(void)
{
    return (volatile uint64_t *)(shared_region_vaddr + SHARED_LOCK_OFFSET);
}

/*
 * Optional CPU hint while spinning.
 * On AArch64, yield tells the CPU this is a spin-wait loop.
 */
static inline void
cpu_relax(void)
{
    __asm__ volatile("yield" ::: "memory");
}

/*
 * Try to acquire the lock once.
 *
 * Return:
 *   1: acquired
 *   0: failed
 *
 * This uses:
 *   ldaxr: acquire-load exclusive
 *   stxr : store exclusive
 *
 * If the lock is already 1, acquisition fails.
 * If the lock is 0, we try to atomically write 1.
 */
static inline int
shared_lock_try_acquire(void)
{
    volatile uint64_t *lock = shared_lock_addr();

    uint64_t old;
    uint32_t failed;

    __asm__ volatile(
        "ldaxr  %0, [%2]\n"
        "cbnz   %0, 1f\n"
        "mov    %0, %3\n"
        "stxr   %w1, %0, [%2]\n"
        "b      2f\n"
        "1:\n"
        "mov    %w1, #1\n"
        "2:\n"
        : "=&r"(old), "=&r"(failed)
        : "r"(lock), "r"(SHARED_LOCK_LOCKED)
        : "memory"
    );

    return failed == 0;
}

/*
 * Busy-poll until the lock is acquired.
 */
static void
shared_lock_acquire(void)
{
    volatile uint64_t *lock = shared_lock_addr();

    for (;;) {
        if (shared_lock_try_acquire()) {
            return;
        }

        /*
         * Wait while the lock is visibly held.
         * This avoids repeatedly issuing exclusive stores.
         */
        while (*lock == SHARED_LOCK_LOCKED) {
            cpu_relax();
        }
    }
}

/*
 * Release the lock.
 *
 * stlr gives release semantics:
 * all writes in the critical section become visible before the lock is released.
 */
static void
shared_lock_release(void)
{
    volatile uint64_t *lock = shared_lock_addr();

    __asm__ volatile(
        "stlr %1, [%0]\n"
        :
        : "r"(lock), "r"(SHARED_LOCK_UNLOCKED)
        : "memory"
    );
}

/*
 * Initialise the lock.
 *
 * Important:
 * only one core / thread should call this before other threads may acquire it.
 * If multiple threads call this concurrently, one thread may reset the lock
 * while another already holds it.
 */
static void
shared_lock_init(void)
{
    volatile uint64_t *lock = shared_lock_addr();

    *lock = SHARED_LOCK_UNLOCKED;

    /*
     * Ensure the zero write is globally visible before later use.
     */
    __asm__ volatile("dmb ish" ::: "memory");
}

static inline uint64_t read_cntvct(void)
{
    uint64_t v;
    asm volatile("isb" ::: "memory");
    asm volatile("mrs %0, cntvct_el0" : "=r"(v));
    return v;
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    // if you comment out the shared lock calls, you will likely see interleaved prints from both cores
    // shared_lock_acquire();
#if 0
        switch (microkit_msginfo_get_label(msginfo)) {
        case 1:
            microkit_dbg_puts("SERVER_SHARED: received a ppc call from channel ");
            if (ch == CORE_0_CH) {
                microkit_dbg_puts("0\n");
            } else if (ch == CORE_1_CH) {
                microkit_dbg_puts("1\n");
            } else {
                microkit_dbg_puts("unknown\n");
            }
            break;
        default:
            microkit_dbg_puts("SERVER_SHARED|ERROR: received an unexpected message\n");
        }
#else
#endif
    // shared_lock_release();

    cycles_t curr = 0;
    cycles_t last = 0;
    cycles_t gap = 0;
    cycles_t start = 0;

    for (;;) {
        start = read_cntvct() * 50;
        if (start >= 20000000) {
            break;
        }
    }
    // seL4_Word local_cnt = 0;

    RECORDING_BEGIN()

    seL4_Word badge;
    seL4_MessageInfo_t tag UNUSED;
    seL4_MessageInfo_t reply_tag = microkit_msginfo_new(0, 0);
    /* To make this simpler this literally just always replies */
    while (1) {
        shared_lock_acquire();
        {
            curr = read_cntvct() * 50;
            if (last != 0) {
                gap = curr - last;
            // } else {
            //     start = curr;
            }
            if (gap != 0) {
                RECORDING_ADD_SAMPLE(last, curr)
            }
            last = curr;
            seL4_Word *shared_data = shared_data_region_addr(0);
            if ((*shared_data > 1000000)) {
                seL4_SetMR(0, (*shared_data));
                seL4_SetMR(1, start);
                reply_tag = microkit_msginfo_new(0, 2);
            } else {
                seL4_SetMR(0, (*shared_data));
            }
            (*shared_data)++;
        }
        shared_lock_release();

        /* the reason we put the INPUT_CAP here is the call comes from it */
        tag = seL4_ReplyRecv(INPUT_CAP, reply_tag, &badge, REPLY_CAP);
    }

    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    /*
     * Only do this if this PD/core is responsible for initialising the shared
     * lock before anyone else can use it.
     */
    shared_lock_init();
    pmu_enable();
    write_shared_data(SUM_OFFSET, 0);
    write_shared_data(SUM_SQUARED_OFFSET, 0);
    write_shared_data(MIN_OFFSET, CYCLES_MAX);
    write_shared_data(MAX_OFFSET, CYCLES_MIN);
    write_shared_data(SAMPLE_OFFSET, 0);
    write_shared_data(LAST_START_OFFSET, 0);
    write_shared_data(LAST_END_OFFSET, 0);
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");
}

void notified(microkit_channel ch) {}