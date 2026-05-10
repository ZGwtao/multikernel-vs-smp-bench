/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include "benchmark.h"

#define SERVER_CH 10
#define REMOTE_CH 2

void init(void) {}

void notified(microkit_channel ch)
{
    for (;;) {
        (void) microkit_ppcall(SERVER_CH, microkit_msginfo_new(0, 0));
    }
}
