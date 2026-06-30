#include "sel4-smp-lock.h"

clh_lock_t *big_kernel_lock;

BOOT_CODE void clh_lock_init(void)
{
    /* Check if linker honoured alignment */
    if (!(((seL4_Word)big_kernel_lock) % EXCL_RES_GRANULE_SIZE == 0)) {
        volatile int *p = (volatile int *)0;
        *p = 5;
        asm volatile("" ::: "memory");
    };

    for (int i = 0; i < CONFIG_MULTIKERNEL_NUM_CPUS; i++) {
        big_kernel_lock->node[i].myreq = &big_kernel_lock->request[i];
    }

    /* Initialize the CLH tail */
    big_kernel_lock->request[CONFIG_MULTIKERNEL_NUM_CPUS].state = CLHState_Granted;
    big_kernel_lock->tail = &big_kernel_lock->request[CONFIG_MULTIKERNEL_NUM_CPUS];

    /* TODO: ?? Kernel missing a barrier (fine because of other barriers though) */
    asm volatile("dmb ish" ::: "memory");
}


void handleIPI(irq_t irq, bool_t irqPath)
{
    /* Stubbed out to do nothing */
    asm volatile("nop" ::: "memory");
}
