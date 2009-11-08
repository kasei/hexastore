#ifndef __TIMING_H__
#define __TIMING_H__

#include <stdio.h>
#include <inttypes.h>
#include "timing_choices.h"

#ifndef TIMING_USE
#define TIMING_USE TIMING_TIME
#endif

#define ENABLE_TIME_LOGGING 1

#if TIMING_USE == TIMING_NONE
#define TIME_T(...)
#define TIME()
#define DIFFTIME(b,a)
#define PRINTTIME
#define TIMEUNITS
#elif TIMING_USE == TIMING_TIME
#include <time.h>
#define TIME_T(...) time_t __VA_ARGS__
#define TIME() time(NULL)
#define DIFFTIME(b,a) difftime(b,a)
#define PRINTTIME PRIu32
#define TIMEUNITS "seconds"
#elif TIMING_USE == TIMING_CLOCK
#include <time.h>
#define TIME_T(...) clock_t __VA_ARGS__
#define TIME() clock()
#define DIFFTIME(b,a) (b-a)
#define PRINTTIME PRIu32
#define TIMEUNITS "seconds"
#elif TIMING_USE == TIMING_RDTSC
#include "mentok/misc/rdtsc.h"
#define TIME_T(...) unsigned long long __VA_ARGS__
#define TIME() rdtsc()
#define PRINTTIME PRId64
#ifdef TIMING_CPU_FREQUENCY
#define DIFFTIME(b,a) ((b-a)/TIMING_CPU_FREQUENCY)
#define TIMEUNITS "seconds"
#else
#define DIFFTIME(b,a) (b-a)
#define TIMEUNITS "cycles"
#endif
#else
#error "Invalid value defined for TIMING_USE."
#endif

#endif /* __TIMING_H__ */
