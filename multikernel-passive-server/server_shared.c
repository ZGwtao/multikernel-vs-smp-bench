/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

#include "benchmark.h"
#include "sel4-smp-lock.h"

#define CORE_0_CH 5
#define CORE_1_CH 6

uintptr_t shared_region_vaddr;
uintptr_t data_region_vaddr;

#define SHARED_TIME_STAMP_OFFSET 8UL

static volatile inline seL4_Word *shared_data_region_addr(seL4_Word index)
{
    return (seL4_Word *)(data_region_vaddr + index);
}

static volatile inline seL4_Word *shared_time_stamp_addr(void)
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

#define SHARED_LOCK_OFFSET      0UL
_Static_assert(sizeof(clh_lock_t) == 2048UL);

#define CPU_COUNT_OFFSET     4088UL

/* clh lock is 2048 */
static inline volatile uint64_t *
shared_lock_addr(void)
{
    return (volatile uint64_t *)(shared_region_vaddr + SHARED_LOCK_OFFSET);
}

static inline volatile uint64_t *
boot_cpu_count_addr(void)
{
    return (volatile uint64_t *)(shared_region_vaddr + CPU_COUNT_OFFSET);
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
    seL4_MessageInfo_t reply_tag = microkit_msginfo_new(0, 1);
    /* To make this simpler this literally just always replies */
    while (1) {
        NODE_LOCK_SYS;
        {
            // puts(microkit_name);
            // puts("\n");
            curr = read_cntvct() * 50;
            if (last != 0) {
                gap = curr - last;
            }
            if (gap != 0) {
                RECORDING_ADD_SAMPLE(last, curr)
            }
            last = curr;
            seL4_Word volatile *shared_data = shared_data_region_addr(0);
            if ((*shared_data >= (WEAK_SCALING_LOOP - 100))) {
                seL4_SetMR(0, (*shared_data));
                seL4_SetMR(1, start);
                reply_tag = microkit_msginfo_new(0, 2);
            } else {
                seL4_SetMR(0, (*shared_data));
            }
            // puthex64((*shared_data));
            // puts("\n");
            (*shared_data)++;
        }
        NODE_UNLOCK_IF_HELD;

        /* the reason we put the INPUT_CAP here is the call comes from it */
        tag = seL4_ReplyRecv(INPUT_CAP, reply_tag, &badge, REPLY_CAP);
    }

    return seL4_MessageInfo_new(0, 0, 0, 0);
}

void init(void)
{
    /* local var pointer to global */
    big_kernel_lock = (clh_lock_t *)shared_lock_addr();

    // NAME: 'server_coreX'
    int core = microkit_name[11] - '0';
    asm volatile("msr tpidr_el0, %0" : : "r"(core));
    asm volatile("isb" ::: "memory");

    pmu_enable();

    /* Copying seL4 release_secondary_cpus() */

    if (core == 0) {
        /* ksNumCPUs = 1 */
        *boot_cpu_count_addr() = 1;

        puts("SERVER_SHARED|INFO: init function running\n");

        clh_lock_init();

        /* node_boot_lock is _Atomic, perform equivalent store/loads */
        // assert(0 == *__atomic_load_n(boot_release_lock_addr(), __ATOMIC_SEQ_CST));
        __atomic_store_n(boot_release_lock_addr(), 1, __ATOMIC_SEQ_CST);

        /* wait for others to be ready */
        /* Wait until the other cores are done initialising */
        // XX: not num cores as only two servers
        while (*boot_cpu_count_addr() != 2) {
            /* perform a memory acquire to get new values of ksNumCPUs, release for ksCurTime */
            __atomic_thread_fence(__ATOMIC_ACQ_REL);
        }

        NODE_LOCK_SYS;
        puts("SERVER_SHARED|INFO: GO GO GO\n");
        NODE_UNLOCK_IF_HELD;

        __atomic_store_n(boot_release2_lock_addr(), 1, __ATOMIC_SEQ_CST);
    } else {
        /* like try_init_kernel_secondary_core*/
        while (__atomic_load_n(boot_release_lock_addr(), __ATOMIC_SEQ_CST) == 0);

        NODE_LOCK_SYS;

        puts("SERVER ");
        puts(microkit_name);
        puts(" has started inside lock\n");

        *boot_cpu_count_addr() = *boot_cpu_count_addr() + 1;
        puts("SERVER_SHARED|INFO: cpu count now");
        puthex64(*boot_cpu_count_addr());
        puts("\n");

        NODE_UNLOCK_IF_HELD;

        while (__atomic_load_n(boot_release2_lock_addr(), __ATOMIC_SEQ_CST) == 0);
    }
}

void notified(microkit_channel ch) {}
