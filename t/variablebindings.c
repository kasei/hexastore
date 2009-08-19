#include <unistd.h>
#include "hexastore.h"
#include "variablebindings.h"
#include "nodemap.h"
#include "node.h"
#include "tap.h"

void _add_data ( hx_hexastore* hx );
hx_variablebindings_iter* _get_triples ( hx_hexastore* hx, int sort );

hx_node* p1;
hx_node* p2;
hx_node* r1;
hx_node* r2;
hx_node* l1;
hx_node* l2;

void vb_test1 ( void );
void vb_freezethaw_test ( void );

int main ( void ) {
	plan_tests(19);
	p1	= hx_new_node_resource( "p1" );
	p2	= hx_new_node_resource( "p2" );
	r1	= hx_new_node_resource( "r1" );
	r2	= hx_new_node_resource( "r2" );
	l1	= hx_new_node_literal( "l1" );
	l2	= hx_new_node_literal( "l2" );
	
	vb_test1();
//	vb_freezethaw_test();
	
	hx_free_node( p1 );
	hx_free_node( p2 );
	hx_free_node( r1 );
	hx_free_node( r2 );
	hx_free_node( l1 );
	hx_free_node( l2 );

	return exit_status();
}

void vb_test1 ( void ) {
	{
		fprintf( stdout, "# testing hx_variablebindings_node_id_for_binding(_name)?\n" );
		char* names[2]			= { "subj", "pred" };
		hx_node_id* nodes		= (hx_node_id*) calloc( 2, sizeof( hx_node_id ) );
		nodes[0]				= 1;
		nodes[1]				= 7;
		hx_variablebindings* b	= hx_new_variablebindings ( 2, names, nodes );
		
		ok1( hx_variablebindings_size(b) == 2 );
		ok1( hx_variablebindings_node_id_for_binding(b,0) == 1 );
		ok1( hx_variablebindings_node_id_for_binding(b,1) == 7 );
		ok1( hx_variablebindings_node_id_for_binding_name(b,"pred") == 7 );
		ok1( hx_variablebindings_node_id_for_binding_name(b,"subj") == 1 );
		
		hx_free_variablebindings(b);
	}
	
	{
		fprintf( stdout, "# testing hx_variablebindings_project\n" );
		int pcols[]				= { 1 };
		char* names[2]			= { "subj", "pred" };
		hx_node_id* nodes		= (hx_node_id*) calloc( 2, sizeof( hx_node_id ) );
		nodes[0]				= 1;
		nodes[1]				= 7;
		hx_variablebindings* b	= hx_new_variablebindings( 2, names, nodes );

		ok1( hx_variablebindings_size(b) == 2 );
		hx_variablebindings* p	= hx_variablebindings_project( b, 1, pcols );
		ok1( hx_variablebindings_size(p) == 1 );
		ok1( hx_variablebindings_node_id_for_binding(p,0) == 7 );
		ok1( hx_variablebindings_node_id_for_binding(p,1) == 0 );
		ok1( hx_variablebindings_node_id_for_binding_name(p,"pred") == 7 );
		ok1( hx_variablebindings_node_id_for_binding_name(p,"subj") == 0 );
		
		hx_free_variablebindings(b);
		hx_free_variablebindings(p);
	}
	
	{
		fprintf( stdout, "# testing hx_variablebindings_project_names\n" );
		char* pnames[]			= { "pred", "badName" };
		char* names[2]			= { "subj", "pred" };
		hx_node_id* nodes		= (hx_node_id*) calloc( 2, sizeof( hx_node_id ) );
		nodes[0]				= 1;
		nodes[1]				= 7;
		hx_variablebindings* b	= hx_new_variablebindings( 2, names, nodes );

		ok1( hx_variablebindings_size(b) == 2 );
		hx_variablebindings* p	= hx_variablebindings_project_names( b, 2, pnames );
		ok1( hx_variablebindings_size(p) == 2 );
		ok1( hx_variablebindings_node_id_for_binding(p,0) == 7 );
		ok1( hx_variablebindings_node_id_for_binding(p,1) == 0 );	// project on bad name, should have a 0-valued node
		ok1( hx_variablebindings_node_id_for_binding_name(p,"pred") == 7 );
		ok1( hx_variablebindings_node_id_for_binding_name(p,"subj") == 0 ); // doesn't exist, should return a 0-valued node
		
		char** getnames	= hx_variablebindings_names( p );
		ok1( strcmp( getnames[0], "pred" ) == 0 );
		ok1( getnames[1] == NULL );	// bad project name (node values will always be 0), but we want to keep it around
		
		hx_free_variablebindings(b);
		hx_free_variablebindings(p);
	}
}

// XXX this needs to be updated to use the new freeze/thaw API that requires use of a nodemap:
// void vb_freezethaw_test ( void ) {
// 	fprintf( stdout, "# testing hx_variablebindings_freeze / hx_variablebindings_thaw\n" );
// 	char* names[2]			= { "subj", "pred" };
// 	hx_node_id* nodes		= (hx_node_id*) calloc( 2, sizeof( hx_node_id ) );
// 	nodes[0]				= 1;
// 	nodes[1]				= 7;
// 	hx_variablebindings* b	= hx_new_variablebindings( 2, names, nodes );
// 	
// 	int len;
// 	char* frozen			= hx_variablebindings_freeze( b, &len );
// 	hx_free_variablebindings(b);
// 	
// 	hx_variablebindings* thawed	= hx_variablebindings_thaw( frozen, len );
// 	
// 	ok1( hx_variablebindings_size(thawed) == 2 );
// 	ok1( hx_variablebindings_node_id_for_binding(thawed,0) == 1 );
// 	ok1( hx_variablebindings_node_id_for_binding(thawed,1) == 7 );
// 	ok1( hx_variablebindings_node_id_for_binding_name(thawed,"pred") == 7 );
// 	ok1( hx_variablebindings_node_id_for_binding_name(thawed,"subj") == 1 );
// 	
// 	hx_free_variablebindings(thawed);
// }
