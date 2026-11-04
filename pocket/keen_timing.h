/* Original VGA retrace delays are distinct from PIT gameplay/speaker ticks.
 * Reference: Chocolate Keen constants.h/engine_timing.c. SPDX-License-Identifier: MIT */
#ifndef KEEN_TIMING_H
#define KEEN_TIMING_H
#include <stdint.h>
#define KT_PIT_HZ 1193182u
#define KT_PIT_DIVISOR 8192u
#define KT_VGA_REFRESH_SCALED 70086u
#define KT_REFRESH_DENOMINATOR ((uint64_t)KT_PIT_HZ*1000u)
static inline uint64_t kt_refresh_units(unsigned pit_ticks){return (uint64_t)pit_ticks*KT_PIT_DIVISOR*KT_VGA_REFRESH_SCALED;}
static inline int kt_delay_elapsed(unsigned pit_ticks,unsigned refresh_frames){return kt_refresh_units(pit_ticks)>=(uint64_t)refresh_frames*KT_REFRESH_DENOMINATOR;}
#endif
