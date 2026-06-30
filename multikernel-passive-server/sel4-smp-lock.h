/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/**
 * All of the code in this file is taken directly from seL4 itself,
 * cut and pasted from multiple different files.
 **/

#pragma once

#include <sel4/sel4.h>
#include <stdbool.h>

#define ALIGN(n)  __attribute__((__aligned__(n)))
#define BIT(n)  (1UL << (n))
#define FORCE_INLINE __attribute__((always_inline))

#define L1_CACHE_LINE_SIZE_BITS CONFIG_L1_CACHE_LINE_SIZE_BITS
#define L1_CACHE_LINE_SIZE BIT(L1_CACHE_LINE_SIZE_BITS)

/*
 * Used to align the big kernel lock to the exclusive reservation granule size.
 * Without this nearby writes can delay atomic operations implemented with looping
 * exclusive load/store instructions for an undefined time.
 *
 * Usually equal to L1_CACHE_LINE_SIZE, but 2k is the maximum for SMP systems.
 *
 * ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition, chapter A3.4.5
 * Load-Exclusive and Store-Exclusive usage restrictions, page 122 states:
 *
 * "The architecture sets an upper limit of 2048 bytes on the size of a region
 *  that can be marked as exclusive."
 *
 * ARM Architecture Reference Manual ARMv8 for A-profile architecture, chapter
 * B2.9.5 Load-Exclusive and Store-Exclusive instruction usage restrictions,
 * page 216 states:
 *
 * "The architecture sets an upper limit of 2048 bytes on the Exclusives
 *  reservation granule that can be marked as exclusive."
 */
#define EXCL_RES_GRANULE_SIZE 2048

typedef seL4_Word word_t;
typedef bool bool_t;

typedef enum {
    IpiRemoteCall_Stall,
    IpiRemoteCall_InvalidateTranslationSingle,
    IpiRemoteCall_InvalidateTranslationASID,
    IpiRemoteCall_InvalidateTranslationAll,
    IpiRemoteCall_switchFpuOwner,
    IpiRemoteCall_MaskPrivateInterrupt,
#ifdef CONFIG_ARM_GIC_V3_SUPPORT
    IpiRemoteCall_DeactivatePrivateInterrupt,
#endif
#ifdef CONFIG_ARM_HYPERVISOR_SUPPORT
    IpiRemoteCall_VCPUInjectInterrupt,
#endif
    /* Add relevant calls here upon required */
    IpiNumArchRemoteCall
} IpiRemoteCall_t;

#define MAX_IPI_ARGS    3   /* Maximum number of parameters to remote function */

typedef struct {
    word_t count;                   /* IPI barrier for remote call synchronization */
    word_t globalsense;
    word_t totalCoreBarrier;        /* number of cores involved in IPI 'in progress' */
    IpiRemoteCall_t remoteCall;     /* the remote call being requested */
    word_t args[MAX_IPI_ARGS];      /* data to be passed to the remote call function */
} ipi_state_t;

/* Use the first two SGI (Software Generated Interrupt) IDs
 * for seL4 IPI implementation. SGIs are per-core banked.
 */
#define irq_remote_call_ipi        0
#define irq_reschedule_ipi         1

static inline void arch_pause(void)
{
    /* TODO */
}

typedef word_t cpu_id_t;

#define KERNEL_STACK_ALIGNMENT 4096
#define CPUID_MASK  (KERNEL_STACK_ALIGNMENT - 1)

static inline CONST cpu_id_t
getCurrentCPUIndex(void)
{
    cpu_id_t id;
    asm volatile("mrs %0, tpidr_el0" : "=r"(id));
    return (id & CPUID_MASK);
}

#define CORE_IRQ_TO_IRQT(tgt, _irq) ((irq_t){.irq = (_irq), .target_core = (tgt)})

typedef struct {
    word_t irq;
    word_t target_core;
} irq_t;

void handleIPI(irq_t irq, bool_t irqPath);

#define BOOT_CODE


/** ================= From here begins the lock ===================== */

/* CLH lock is FIFO lock for machines with coherent caches (coherent-FIFO lock).
 * See ftp://ftp.cs.washington.edu/tr/1993/02/UW-CSE-93-02-02.pdf */

typedef enum {
    CLHState_Granted = 0,
    CLHState_Pending
} clh_req_state_t;

/* Lock request */
typedef struct clh_req {
    clh_req_state_t state;
} ALIGN(L1_CACHE_LINE_SIZE) clh_req_t;

/* Our node (called "Process" in the paper) */
typedef struct clh_node {
    clh_req_t *watch; // Used by predecessor to grant the lock to us.
    clh_req_t *myreq; // Used to grant the lock to our successor.
    /* This is the software blocking IPI flag */
    word_t ipi;
} ALIGN(L1_CACHE_LINE_SIZE) clh_node_t;

typedef struct clh_lock {
    clh_req_t request[CONFIG_MULTIKERNEL_NUM_CPUS + 1];
    clh_node_t node[CONFIG_MULTIKERNEL_NUM_CPUS];

    clh_req_t *tail;

    /* Global IPI state */
    ipi_state_t ipi;
} ALIGN(EXCL_RES_GRANULE_SIZE) clh_lock_t;

/* modified: pointer as can't do easily not-pointer */
extern clh_lock_t *big_kernel_lock;

BOOT_CODE void clh_lock_init(void);

static inline bool_t FORCE_INLINE clh_is_ipi_pending(word_t cpu)
{
    /* Asssure IPI data is accessed only when this flag is set */
    return __atomic_load_n(&big_kernel_lock->node[cpu].ipi, __ATOMIC_ACQUIRE);
}

static inline void FORCE_INLINE clh_lock_acquire(bool_t irqPath)
{
    word_t cpu = getCurrentCPUIndex();
    clh_node_t *node = &big_kernel_lock->node[cpu];

    /* Tell successor to wait */
    node->myreq->state = CLHState_Pending;
    /* Enqueue our request */
    node->watch = __atomic_exchange_n(&big_kernel_lock->tail, node->myreq, __ATOMIC_ACQ_REL);

    /* Wait until predecessor finishes */
    while (node->watch->state != CLHState_Granted) {
        /* As we are in a loop we need to ensure that any loads of future iterations of the
         * loop are performed after this one */
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if (clh_is_ipi_pending(cpu)) {
            /* we only handle irq_remote_call_ipi here as other type of IPIs
             * are async and could be delayed. 'handleIPI' may not return
             * based on value of the 'irqPath'. */
            handleIPI(CORE_IRQ_TO_IRQT(cpu, irq_remote_call_ipi), irqPath);
            /* We do not need to perform a memory release here as we would have only modified
             * local state that we do not need to make visible */
        }
        arch_pause();
    }

    /* make sure no resource access passes from this point */
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}

static inline void FORCE_INLINE clh_lock_release(void)
{
    clh_node_t *node = &big_kernel_lock->node[getCurrentCPUIndex()];

    /* make sure no resource access passes from this point */
    __atomic_thread_fence(__ATOMIC_RELEASE);

    /* Pass lock to successor */
    node->myreq->state = CLHState_Granted;
    /* Take ownership of watched request, to use next time we take the lock */
    node->myreq = node->watch;
}

static inline bool_t FORCE_INLINE clh_is_self_in_queue(void)
{
    return big_kernel_lock->node[getCurrentCPUIndex()].myreq->state == CLHState_Pending;
}

#define NODE_LOCK(_irqPath) do {                         \
    clh_lock_acquire(_irqPath);                          \
} while(0)

#define NODE_UNLOCK do {                                 \
    clh_lock_release();                                  \
} while(0)

#define NODE_LOCK_IF(_cond, _irqPath) do {               \
    if((_cond)) {                                        \
        NODE_LOCK(_irqPath);                             \
    }                                                    \
} while(0)

#define NODE_UNLOCK_IF_HELD do {                         \
    if(clh_is_self_in_queue()) {                         \
        NODE_UNLOCK;                                     \
    }                                                    \
} while(0)

#define NODE_LOCK_SYS NODE_LOCK(false)
#define NODE_LOCK_IRQ NODE_LOCK(true)
#define NODE_LOCK_SYS_IF(_cond) NODE_LOCK_IF(_cond, false)
#define NODE_LOCK_IRQ_IF(_cond) NODE_LOCK_IF(_cond, true)

