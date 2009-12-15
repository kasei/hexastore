#include "mentok/engine/delay.h"

int _hx_delay_debug ( void* data, char* header, int _indent );
int _hx_delay_start ( _hx_delay_iter_vb_info* info );

// implementations

int _hx_delay_iter_vb_finished ( void* data ) {
//	fprintf( stderr, "*** _hx_delay_iter_vb_finished (%p)\n", (void*) data );
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_delay_start( info );
	}
	return hx_variablebindings_iter_finished( info->iter );
}

int _hx_delay_iter_vb_current ( void* data, void* results ) {
//	fprintf( stderr, "*** _hx_delay_iter_vb_current\n" );
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_delay_start( info );
	}
	return hx_variablebindings_iter_current( info->iter, results );
}

int _hx_delay_iter_vb_next ( void* data ) {
// 	fprintf( stderr, "*** _hx_delay_iter_vb_next\n" );
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	if (info->started == 0) {
		_hx_delay_start( info );
	} else {
		info->total++;
	}
	
	
	double required_ms_delay	= (1.0 / info->results_per_second) * 1000.0;
	fprintf( stderr, "%lf results/second means a delay of %lfms between results\n", info->results_per_second, required_ms_delay );
	
	struct timeval cur_time, elapsedt;
	int status;
	status = gettimeofday (&cur_time, NULL);
	fprintf( stderr, "current time %ld:%ld\n", (long) cur_time.tv_sec, (long) cur_time.tv_usec );
	double cur_ms		= (1000.0 * cur_time.tv_sec - info->last_time.tv_usec / 1000.0);
	double last_ms		= (1000.0 * info->last_time.tv_sec - info->last_time.tv_usec / 1000.0);
	double elapsed_ms	= (cur_ms - last_ms);
	fprintf( stderr, "elapsed time %lfms\n", elapsed_ms );
	if (elapsed_ms < required_ms_delay) {
		double needed_ms_delay	= required_ms_delay - elapsed_ms;
		fprintf( stderr, "delaying %lfms (%d results, %lf elapsed ms)\n", needed_ms_delay, (int) info->total, elapsed_ms );
		usleep( needed_ms_delay * 1000 );
	}
	info->last_time	= cur_time;
	return hx_variablebindings_iter_next( info->iter );
}

int _hx_delay_iter_vb_free ( void* data ) {
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	return hx_free_variablebindings_iter( info->iter );
}

int _hx_delay_iter_vb_size ( void* data ) {
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	return hx_variablebindings_iter_size( info->iter );
}

char** _hx_delay_iter_vb_names ( void* data ) {
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	return hx_variablebindings_iter_names( info->iter );
}

int _hx_delay_iter_sorted_by ( void* data, int index ) {
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	return hx_variablebindings_iter_is_sorted_by_index( info->iter, index );
}

hx_variablebindings_iter* hx_new_delay_iter ( hx_variablebindings_iter* iter, long latency, double results_per_second ) {
	hx_variablebindings_iter_vtable* vtable	= (hx_variablebindings_iter_vtable*) malloc( sizeof( hx_variablebindings_iter_vtable ) );
	if (vtable == NULL) {
		fprintf( stderr, "*** malloc failed in hx_new_delay_iter\n" );
		return NULL;
	}
	vtable->finished	= _hx_delay_iter_vb_finished;
	vtable->current		= _hx_delay_iter_vb_current;
	vtable->next		= _hx_delay_iter_vb_next;
	vtable->free		= _hx_delay_iter_vb_free;
	vtable->names		= _hx_delay_iter_vb_names;
	vtable->size		= _hx_delay_iter_vb_size;
	vtable->sorted_by_index	= _hx_delay_iter_sorted_by;
	vtable->debug		= _hx_delay_debug;
	
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) calloc( 1, sizeof( _hx_delay_iter_vb_info ) );
	info->started				= 0;
	info->total					= 0;
	info->results_per_second	= results_per_second;
	info->latency				= latency;
	info->iter					= iter;
	
	fprintf( stderr, "new delay iterator with latency=%ldms and %lf results/second\n", latency, results_per_second );
	
	hx_variablebindings_iter* diter	= hx_variablebindings_new_iter( vtable, (void*) info );
	return diter;
}

void hx_delay_iter_debug ( hx_variablebindings_iter* iter ) {
	fprintf( stderr, "Materialized iterator %p\n", (void*) iter );
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) iter->ptr;
	fprintf( stderr, "\tInfo: %p\n", (void*) info );
	fprintf( stderr, "\tLatency: %ld\n", info->latency );
	fprintf( stderr, "\tResults per second: %lf\n", info->results_per_second );
	hx_variablebindings_iter_debug( info->iter, " ", 0 );
}

int _hx_delay_debug ( void* data, char* header, int _indent ) {
	_hx_delay_iter_vb_info* info	= (_hx_delay_iter_vb_info*) data;
	char* indent	= (char*) malloc( _indent + 1 );
	if (indent == NULL) {
		fprintf( stderr, "*** malloc failed in _hx_delay_debug\n" );
		return -1;
	}
	char* p			= indent;
	int i;
	for (i = 0; i < _indent; i++) *(p++) = ' ';
	*p	= (char) 0;
	
	fprintf( stderr, "%s%s delay iterator\n", indent, header );
	
	fprintf( stderr, "%s%s  Info: %p\n", indent, header, (void*) info );
	fprintf( stderr, "\tLatency: %ld\n", info->latency );
	fprintf( stderr, "\tResults per second: %lf\n", info->results_per_second );
	hx_variablebindings_iter_debug( info->iter, header, _indent );
	free(indent);
	return 0;
}

int _hx_delay_start ( _hx_delay_iter_vb_info* info ) {
	if (info->started == 0) {
		info->started	= 1;
		info->total++;
		usleep( 1000 * info->latency );
		gettimeofday (&(info->last_time), NULL);
		fprintf( stderr, "started at %ld:%ld\n", (long) info->last_time.tv_sec, (long) info->last_time.tv_usec );
	}
}
