#include "unistd.h"
#include "tcindex.h"
#include "tap.h"

void spo_test1 ( void );
void spo_test2 ( void );
void pos_iter_test1 ( void );
void spo_iter_test1 ( void );
void pso_iter1_test1 ( void );

int main ( void ) {
	plan_tests(128);
	
	spo_test1();
	spo_test2();
	
	pos_iter_test1();
	spo_iter_test1();
	pso_iter1_test1();
	
	return exit_status();
}

void spo_test1 ( void ) {
	fprintf( stderr, "# spo_test1\n" );
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_tcindex* i	= hx_new_tcindex( st, HX_TCINDEX_ORDER_SPO, "/tmp/tcindex.tcb" );
	ok1( i != NULL );
	
	hx_node_id s	= (hx_node_id) 1;
	hx_node_id p	= (hx_node_id) 2;
	hx_node_id o	= (hx_node_id) 3;
	
	ok1( hx_tcindex_triples_count( i, st ) == 0 );
	ok1( hx_tcindex_add_triple( i, st, s, p, o ) == 0 );
	ok1( hx_tcindex_triples_count( i, st ) == 1 );
	ok1( hx_tcindex_add_triple( i, st, s, p, o ) == 1 ); //adding duplicate triple returns non-zero
	
	o	= (hx_node_id) 4;
	ok1( hx_tcindex_add_triple( i, st, s, p, o ) == 0 );
	ok1( hx_tcindex_triples_count( i, st ) == 2 );
	ok1( hx_tcindex_remove_triple( i, st, s, p, (hx_node_id) 3 ) == 0 );
	ok1( hx_tcindex_triples_count( i, st ) == 1 );
	ok1( hx_tcindex_remove_triple( i, st, s, p, (hx_node_id) 4 ) == 0 );
	ok1( hx_tcindex_triples_count( i, st ) == 0 );
	
	hx_free_tcindex(i, st);
	hx_free_storage_manager( st );
	unlink( "/tmp/tcindex.tcb" );
}

void spo_test2 ( void ) {
	fprintf( stderr, "# spo_test2\n" );
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_tcindex* i	= hx_new_tcindex( st, HX_TCINDEX_ORDER_SPO, "/tmp/tcindex.tcb" );
	hx_node_id s, p, o;
	s	= (hx_node_id) 1;
	for (p = 1; p <= 10; p++) {
		for (o = 1; o <= 100; o++) {
			hx_tcindex_add_triple( i, st, s, p, o );
		}
	}
	ok1( hx_tcindex_triples_count( i, st ) == 1000 );
	
	for (p = 1; p <= 10; p++) {
		for (o = 1; o <= 50; o++) {
			hx_tcindex_remove_triple( i, st, s, p, o );
		}
	}
	ok1( hx_tcindex_triples_count( i, st ) == 500 );

	for (p = 1; p <= 10; p++) {
		for (o = 26; o <= 100; o++) {
			hx_tcindex_add_triple( i, st, s, p, o );
		}
	}
	ok1( hx_tcindex_triples_count( i, st ) == 750 );
	
	hx_free_tcindex(i, st);
	hx_free_storage_manager( st );
	unlink( "/tmp/tcindex.tcb" );
}

void pos_iter_test1 ( void ) {
	fprintf( stderr, "# pos_iter_test1\n" );
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_tcindex* i	= hx_new_tcindex( st, HX_TCINDEX_ORDER_POS, "/tmp/tcindex.tcb" );
	ok1( i != NULL );
	
	const int range	= 3;
	const int triples	= range * range * range;
	
	hx_node_id s, p, o;
	for (s = 1; s <= range; s++) {
		for (p = 1; p <= range; p++) {
			for (o = 1; o <= range; o++) {
				hx_tcindex_add_triple( i, st, s, p, o );
			}
		}
	}
	ok1( hx_tcindex_triples_count( i, st ) == triples );
	
	{
		hx_tcindex_iter* iter	= hx_tcindex_new_iter( i, st );
		int counter	= 0;
		ok1( iter != NULL );
		hx_node_id last_s, last_p, last_o;
		hx_node_id cur_s, cur_p, cur_o;
		while (!hx_tcindex_iter_finished(iter)) {
			hx_tcindex_iter_current( iter, &cur_s, &cur_p, &cur_o );
			if (counter > 0) {
				ok1( last_p <= cur_p );
				if (counter % range > 0) {
					ok1( last_o == cur_o );
				}
			}
			last_s	= cur_s;
			last_p	= cur_p;
			last_o	= cur_o;
			counter++;
			hx_tcindex_iter_next(iter);
		}
		hx_free_tcindex_iter( iter );
		ok1( counter == triples );
	}
	
	hx_free_tcindex(i, st);
	hx_free_storage_manager( st );
	unlink( "/tmp/tcindex.tcb" );
}

void spo_iter_test1 ( void ) {
	fprintf( stderr, "# spo_iter_test1\n" );
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_tcindex* i	= hx_new_tcindex( st, HX_TCINDEX_ORDER_SPO, "/tmp/tcindex.tcb" );
	ok1( i != NULL );
	
	const int range	= 3;
	const int triples	= range * range * range;
	
	hx_node_id s, p, o;
	for (s = 1; s <= range; s++) {
		for (p = 1; p <= range; p++) {
			for (o = 1; o <= range; o++) {
				hx_tcindex_add_triple( i, st, s, p, o );
			}
		}
	}
	ok1( hx_tcindex_triples_count( i, st ) == triples );
	
	{
		hx_tcindex_iter* iter	= hx_tcindex_new_iter( i, st );
		int counter	= 0;
		ok1( iter != NULL );
		hx_node_id last_s, last_p, last_o;
		hx_node_id cur_s, cur_p, cur_o;
		while (!hx_tcindex_iter_finished(iter)) {
			hx_tcindex_iter_current( iter, &cur_s, &cur_p, &cur_o );
			if (counter > 0) {
				ok1( last_s <= cur_s );
				if (counter % range > 0) {
					ok1( last_p == cur_p );
				}
			}
			last_s	= cur_s;
			last_p	= cur_p;
			last_o	= cur_o;
			counter++;
			hx_tcindex_iter_next(iter);
		}
		hx_free_tcindex_iter( iter );
		ok1( counter == triples );
	}
	
	hx_free_tcindex(i, st);
	hx_free_storage_manager( st );
	unlink( "/tmp/tcindex.tcb" );
}

void pso_iter1_test1 ( void ) {
	fprintf( stderr, "# pso_iter1_test1\n" );
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_tcindex* i	= hx_new_tcindex( st, HX_TCINDEX_ORDER_PSO, "/tmp/tcindex.tcb" );
	ok1( i != NULL );
	
	const int range	= 3;
	const int triples	= range * range * range;
	
	hx_node_id s, p, o;
	for (int j = 0; j < 2; j++) {
		// duplicate adds should do nothing
		for (s = 1; s <= range; s++) {
			for (p = 1; p <= range; p++) {
				for (o = 1; o <= range; o++) {
					hx_tcindex_add_triple( i, st, s, p, o );
				}
			}
		}
	}
	ok1( hx_tcindex_triples_count( i, st ) == triples );
	
	const hx_node_id seek	= (hx_node_id) 2;
	hx_tcindex_iter* iter	= hx_tcindex_new_iter1( i, st, (hx_node_id) -1, seek, (hx_node_id) -2 );
	ok1( iter != NULL );
	hx_node_id last_s, last_p, last_o;
	hx_node_id cur_s, cur_p, cur_o;
	
	int counter	= 0;
	while (!hx_tcindex_iter_finished(iter)) {
		hx_tcindex_iter_current( iter, &cur_s, &cur_p, &cur_o );
		if (counter > 0) {
			ok1( last_s <= cur_s );
			if (counter % range > 0) {
				ok1( last_o <= cur_o );
			}
		}
		last_s	= cur_s;
		last_p	= cur_p;
		last_o	= cur_o;
		counter++;
		hx_tcindex_iter_next(iter);
	}
	hx_free_tcindex_iter( iter );
	ok1( counter == range * range );
	
	hx_free_tcindex(i, st);
	hx_free_storage_manager( st );
	unlink( "/tmp/tcindex.tcb" );
}

