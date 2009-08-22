#include "misc/util.h"
#include "test/tap.h"

void test1 ( void );
void test2 ( void );

void hash_debug_1 ( void* key, void* value );
int apply_test_1( void* key, void* value, void* thunk );

int main ( void ) {
	plan_tests(37);

	test1();
	test2();
	
	return exit_status();
}

void test1 ( void ) {
	hx_hash_t* h	= hx_new_hash( 10 );
	
	hx_hash_add( h, "a", 0, (void*) 1 );
	hx_hash_add( h, "b", 0, (void*) 2 );
	hx_hash_add( h, "c", 0, (void*) 3 );
	
	{
		int count	= hx_hash_apply( h, "a", 1, apply_test_1, NULL );
		ok1( count == 1 );
	}

	{
		int count	= hx_hash_apply( h, "b", 1, apply_test_1, NULL );
		ok1( count == 1 );
	}

	{
		int count	= hx_hash_apply( h, "c", 1, apply_test_1, NULL );
		ok1( count == 1 );
	}
	
	{
		int count	= hx_hash_apply( h, NULL, 0, apply_test_1, NULL );
		ok1( count == 3 );
	}
	
	hx_free_hash( h, NULL );
}

void test2 ( void ) {
	hx_hash_t* h	= hx_new_hash( 2 );
	
	int i;
	for (i = 0; i < 26; i++) {
		char c	= 'a' + i;
		void* v	= (void*) (i + 1);
		char* key	= (char*) malloc(2);
		snprintf( key, 2, "%c", c );
		hx_hash_add( h, key, 0, v );
		free(key);
	}
	
//	hx_hash_debug( h, hash_debug_1 );
	int count	= hx_hash_apply( h, NULL, 0, apply_test_1, NULL );
	ok1( count == 26 );
	
	hx_free_hash( h, NULL );
}

int apply_test_1( void* key, void* value, void* thunk ) {
	char c	= *( (char*) key );
	int v	= (int) value;
	ok1( v == (c - 'a' + 1) );
	return 0;
}

void hash_debug_1 ( void* key, void* value ) {
	fprintf( stderr, "\t(%s => %d)\n", (char*) key, (int) value );
}
