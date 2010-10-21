#ifndef _DELAY_H
#define _DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include "mentok/mentok_types.h"
#include "mentok/engine/variablebindings_iter.h"

typedef struct {
	int started;
	struct timeval last_time;
	long latency;
	double results_per_second;
	uint64_t total;
	hx_variablebindings_iter* iter;
} _hx_delay_iter_vb_info;

int _hx_delay_iter_vb_finished ( void* iter );
int _hx_delay_iter_vb_current ( void* iter, void* results );
int _hx_delay_iter_vb_next ( void* iter );	
int _hx_delay_iter_vb_free ( void* iter );
int _hx_delay_iter_vb_size ( void* iter );
char** _hx_delay_iter_vb_names ( void* iter );

hx_variablebindings_iter* hx_new_delay_iter ( hx_variablebindings_iter* iter, long latency, double results_per_second );

void hx_delay_iter_debug ( hx_variablebindings_iter* iter );

#ifdef __cplusplus
}
#endif

#endif
