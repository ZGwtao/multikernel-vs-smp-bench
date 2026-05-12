#pragma once

#define NUM_WARMUP 10000
#define NUM_SAMPLES 4000000

/* These numbers are used as the channels from the manager POV.
 * If it is defined as 0, it is disabled.
 * If adding a new one, add it to benchmark_start_stop_channels in manager.
*/


#define BENCHMARK_CH 1

// #define CONFIG_PLAT_ODROIDC4 1
#define CONFIG_PLAT_MAAXBOARD 1
// #define CONFIG_PLAT_TQMA8XQP1GB 1
// #define CONFIG_PLAT_BCM2711 1
