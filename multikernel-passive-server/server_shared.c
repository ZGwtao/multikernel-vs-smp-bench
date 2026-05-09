/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>
#include <assert.h>

#include "benchmark.h"

#define CORE_0_CH 5
#define CORE_1_CH 6

uintptr_t shared_region_vaddr;
uintptr_t data_region_vaddr;

#define SHARED_TIME_STAMP_OFFSET 8UL

static inline seL4_Word *shared_data_region_addr(void)
{
    return (seL4_Word *)data_region_vaddr;
}

static inline seL4_Word *shared_time_stamp_addr(void)
{
    return (seL4_Word *)(shared_region_vaddr + SHARED_TIME_STAMP_OFFSET);
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

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo)
{
    // if you comment out the shared lock calls, you will likely see interleaved prints from both cores
    shared_lock_acquire();
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
    {
        seL4_Word *shared_data = shared_data_region_addr();
        if ((*shared_data > 1000000)) {
            for (;;) {}
        }
        
        seL4_Word *time_stamp_addr = shared_time_stamp_addr();
        if ((*time_stamp_addr == 0)) {
            *time_stamp_addr = (seL4_Word)pmu_read_cycles();
        }

        (*shared_data)++;

        if ((*shared_data > 1000000)) {
            cycles_t now = pmu_read_cycles();
            cycles_t start = *time_stamp_addr;
            cycles_t elapsed = now - start;
            print("cycles per increment: ");
            puthex64(elapsed / (*shared_data));
            puts("\n");
            for (;;) {}
        }
    }
#endif
    shared_lock_release();

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
    microkit_dbg_puts("SERVER_SHARED|INFO: init function running\n");
}

void notified(microkit_channel ch) {}