#include "mentok.h"
#include "store/hexastore/hexastore.h"
#include "test/tap.h"

void spo_test1 ( void );
void vb_iter_test1 ( void );
void orderings_test1 ( void );
void orderings_test2 ( void );

int main ( void ) {
	plan_tests(49);
	
	spo_test1();
	vb_iter_test1();
	orderings_test1();
	orderings_test2();
	
	return exit_status();
}

void spo_test1 ( void ) {
	fprintf( stdout, "# spo_test1\n" );
	hx_store* store	= hx_new_store_hexastore( NULL );
	ok1( store != NULL );
	
	hx_node* s		= hx_new_node_resource("s");
	hx_node* p		= hx_new_node_resource("p");
	hx_node* o		= hx_new_node_literal("l");
	hx_node* o2		= hx_new_node_literal("l2");
	hx_triple* t	= hx_new_triple( s, p, o );
	hx_triple* t2	= hx_new_triple( s, p, o2 );
	
	ok1( hx_store_size( store ) == 0 );
	ok1( hx_store_add_triple( store, t ) == 0 );
	ok1( hx_store_size( store ) == 1 );
	ok1( hx_store_add_triple( store, t ) == 0 );
	
	ok1( hx_store_add_triple( store, t2 ) == 0 );
	ok1( hx_store_size( store ) == 2 );
	
	ok1( hx_store_remove_triple( store, t ) == 0 );
	
	ok1( hx_store_size( store ) == 1 );
	ok1( hx_store_remove_triple( store, t2 ) == 0 );
	ok1( hx_store_size( store ) == 0 );

	hx_free_triple(t);
	hx_free_triple(t2);
	hx_free_node(s);
	hx_free_node(p);
	hx_free_node(o);
	hx_free_node(o2);
	hx_free_store(store);
}

void vb_iter_test1 ( void ) {
	fprintf( stdout, "# vb_iter_test1\n" );
	hx_store* store	= hx_new_store_hexastore( NULL );
	hx_node* s		= hx_new_node_resource("s");
	hx_node* s2		= hx_new_node_resource("s2");
	hx_node* s3		= hx_new_node_resource("s3");
	hx_node* p		= hx_new_node_resource("p");
	hx_node* p2		= hx_new_node_resource("p2");
	hx_node* o		= hx_new_node_literal("l");
	hx_node* o2		= hx_new_node_literal("l2");
	hx_node* o3		= hx_new_node_literal("l3");
	
	hx_triple* t	= hx_new_triple( s, p, o );
	hx_triple* t2	= hx_new_triple( s, p, o2 );
	hx_triple* t3	= hx_new_triple( s, p, o3 );
	hx_triple* t4	= hx_new_triple( s, p2, o );
	hx_triple* t5	= hx_new_triple( s, p2, o3 );
	hx_triple* t6	= hx_new_triple( s2, p, o3 );
	hx_triple* t7	= hx_new_triple( s2, p, o2 );
	
	hx_store_add_triple( store, t );
	hx_store_add_triple( store, t2 );
	hx_store_add_triple( store, t3 );
	hx_store_add_triple( store, t4 );
	hx_store_add_triple( store, t5 );
	hx_store_add_triple( store, t6 );
	hx_store_add_triple( store, t7 );

	hx_node* v1		= hx_new_node_named_variable( -1, "x" );
	hx_node* v2		= hx_new_node_named_variable( -2, "y" );
	
	{
		fprintf( stdout, "# { ?x <p> ?y } ORDERED BY ?y\n" );
		hx_triple* tp1	= hx_new_triple( v1, p, v2 );
		hx_variablebindings_iter* iter	= hx_store_get_statements( store, tp1, v2 );
		ok1( iter != NULL );
		int counter	= 0;
		int expected_x[5]	= { 1, 1, 7, 1, 7 };
		int expected_y[5]	= { 3, 4, 4, 5, 5 };
		while (!hx_variablebindings_iter_finished(iter)) {
			hx_variablebindings* b;
			hx_variablebindings_iter_current(iter, &b);
			
			hx_node_id x	= hx_variablebindings_node_id_for_binding_name( b, "x" );
			ok1( x == expected_x[counter] );
			hx_node_id y	= hx_variablebindings_node_id_for_binding_name( b, "y" );
			ok1( y == expected_y[counter] );
			
			counter++;
			hx_variablebindings_iter_next(iter);
		}
		ok1( counter == 5 );
		hx_free_variablebindings_iter( iter );
		hx_free_triple(tp1);
	}

	{
		fprintf( stdout, "# { ?x <p> ?y } ORDERED BY ?x\n" );
		hx_triple* tp1	= hx_new_triple( v1, p, v2 );
		hx_variablebindings_iter* iter	= hx_store_get_statements( store, tp1, v1 );
		ok1( iter != NULL );
		int counter	= 0;
		int expected_x[5]	= { 1, 1, 1, 7, 7 };
		int expected_y[5]	= { 3, 4, 5, 4, 5 };
		while (!hx_variablebindings_iter_finished(iter)) {
			hx_variablebindings* b;
			hx_variablebindings_iter_current(iter, &b);
			
			hx_node_id x	= hx_variablebindings_node_id_for_binding_name( b, "x" );
			ok1( x == expected_x[counter] );
			hx_node_id y	= hx_variablebindings_node_id_for_binding_name( b, "y" );
			ok1( y == expected_y[counter] );
			
			counter++;
			hx_variablebindings_iter_next(iter);
		}
		ok1( counter == 5 );
		hx_free_variablebindings_iter( iter );
		hx_free_triple(tp1);
	}

	{
		fprintf( stdout, "# { ?x <p2> ?y } ORDERED BY ?y\n" );
		hx_triple* tp1	= hx_new_triple( v1, p2, v2 );
		hx_variablebindings_iter* iter	= hx_store_get_statements( store, tp1, v2 );
		ok1( iter != NULL );
		int counter	= 0;
		int expected[2]	= { 3, 5 };
		while (!hx_variablebindings_iter_finished(iter)) {
			hx_variablebindings* b;
			hx_variablebindings_iter_current(iter, &b);
			
			hx_node_id x	= hx_variablebindings_node_id_for_binding_name( b, "x" );
			ok1( x == 1 );
			hx_node_id y	= hx_variablebindings_node_id_for_binding_name( b, "y" );
			ok1( y == expected[counter] );
			
			counter++;
			hx_variablebindings_iter_next(iter);
		}
		ok1( counter == 2 );
		hx_free_variablebindings_iter( iter );
		hx_free_triple(tp1);
	}

	hx_free_node(v1);
	hx_free_node(v2);
	
	hx_free_triple(t);
	hx_free_triple(t2);
	hx_free_triple(t3);
	hx_free_triple(t4);
	hx_free_triple(t5);
	hx_free_triple(t6);
	hx_free_triple(t7);
	
	hx_free_node(s);
	hx_free_node(s2);
	hx_free_node(s3);
	hx_free_node(p);
	hx_free_node(p2);
	hx_free_node(o);
	hx_free_node(o2);
	hx_free_node(o3);
	hx_free_store(store);
}

void orderings_test1 ( void ) {
	fprintf( stdout, "# orderings_test1\n" );
	{
		hx_store* store	= hx_new_store_hexastore( NULL );
		hx_container_t* o	= hx_store_triple_orderings( store, NULL );
		uint64_t index_count	= hx_container_size(o);
		ok1( index_count == 6 );
		hx_free_store(store);
	}
	{
		hx_store* store	= hx_new_store_hexastore_with_indexes( NULL, "spo,pos" );
		hx_container_t* o	= hx_store_triple_orderings( store, NULL );
		uint64_t index_count	= hx_container_size(o);
		ok1( index_count == 2 );
		hx_free_store(store);
	}
}

void orderings_test2 ( void ) {
	fprintf( stdout, "# orderings_test2\n" );
	hx_store* store	= hx_new_store_hexastore_with_indexes( NULL, "pso,pos" );
	hx_model* hx	= hx_new_model_with_store( NULL, store );
	
	hx_node* s		= hx_new_node_resource("s");
	hx_node* s2		= hx_new_node_resource("s2");
	hx_node* s3		= hx_new_node_resource("s3");
	hx_node* p		= hx_new_node_resource("p");
	hx_node* p2		= hx_new_node_resource("p2");
	hx_node* o		= hx_new_node_literal("l");
	hx_node* o2		= hx_new_node_literal("l2");
	hx_node* o3		= hx_new_node_literal("l3");
	
	hx_triple* t	= hx_new_triple( s, p, o );
	hx_triple* t2	= hx_new_triple( s, p, o2 );
	hx_triple* t3	= hx_new_triple( s, p, o3 );
	hx_triple* t4	= hx_new_triple( s, p2, o );
	hx_triple* t5	= hx_new_triple( s, p2, o3 );
	hx_triple* t6	= hx_new_triple( s2, p, o3 );
	hx_triple* t7	= hx_new_triple( s2, p, o2 );
	
	hx_store_add_triple( store, t );
	hx_store_add_triple( store, t2 );
	hx_store_add_triple( store, t3 );
	hx_store_add_triple( store, t4 );
	hx_store_add_triple( store, t5 );
	hx_store_add_triple( store, t6 );
	hx_store_add_triple( store, t7 );
	
	hx_node* v1		= hx_new_node_named_variable( -1, "x" );
	hx_node* v2		= hx_new_node_named_variable( -2, "y" );
	hx_node* v3		= hx_new_node_named_variable( -3, "z" );
	
	{
		hx_triple* t	= hx_new_triple( v1, v2, v3 );
		hx_container_t* o	= hx_store_triple_orderings( store, NULL );
		uint64_t index_count	= hx_container_size(o);
		ok1( index_count == 2 );
		int i;
		for (i = 0; i < index_count; i++) {
			void* index	= hx_container_item( o, i );
			hx_variablebindings_iter* iter	= hx_store_get_statements_with_index(hx->store, t, index);
			int counter	= 0;
			while (!hx_variablebindings_iter_finished(iter)) {
				counter++;
				hx_variablebindings_iter_next(iter);
			}
			ok1( counter == 7 );
		}
	}
	{
		hx_triple* t	= hx_new_triple( v1, p, v3 );
		hx_container_t* o	= hx_store_triple_orderings( store, NULL );
		uint64_t index_count	= hx_container_size(o);
		ok1( index_count == 2 );
		int i;
		for (i = 0; i < index_count; i++) {
			void* index	= hx_container_item( o, i );
			hx_variablebindings_iter* iter	= hx_store_get_statements_with_index(hx->store, t, index);
			int counter	= 0;
			while (!hx_variablebindings_iter_finished(iter)) {
				counter++;
				hx_variablebindings_iter_next(iter);
			}
			ok1( counter == 5 );
		}
	}
	
	
	hx_free_node(v1);
	hx_free_node(v2);
	
	hx_free_triple(t);
	hx_free_triple(t2);
	hx_free_triple(t3);
	hx_free_triple(t4);
	hx_free_triple(t5);
	hx_free_triple(t6);
	hx_free_triple(t7);
	
	hx_free_node(s);
	hx_free_node(s2);
	hx_free_node(s3);
	hx_free_node(p);
	hx_free_node(p2);
	hx_free_node(o);
	hx_free_node(o2);
	hx_free_node(o3);

	hx_free_model(hx);
}
