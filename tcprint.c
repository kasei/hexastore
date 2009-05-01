#include <stdio.h>
#include <stdlib.h>
#include "hexastore.h"
#include "nodemap.h"
#include "storage.h"
#include "node.h"
#include "mergejoin.h"
#include "avl.h"

typedef struct {
	int next;
	struct avl_table* name2id;
	struct avl_table* id2name;
} varmap_t;

typedef struct {
	char* name;
	int id;
} varmap_item_t;

int _varmap_name2id_cmp ( const void* a, const void* b, void* param ) {
	return strcmp( (const char*) a, (const char*) b );
}

int _varmap_id2name_cmp ( const void* a, const void* b, void* param ) {
	varmap_item_t* _a	= (varmap_item_t*) a;
	varmap_item_t* _b	= (varmap_item_t*) b;
	return (_a->id - _b->id);
}

hx_node* node_for_string ( char* string, hx_hexastore* hx );
hx_node_id node_id_for_string ( char* string, hx_hexastore* hx );
hx_node* node_for_string_with_varmap ( char* string, hx_hexastore* hx, varmap_t* varmap );
void print_triple ( hx_nodemap* map, hx_node_id s, hx_node_id p, hx_node_id o, int count );
char* variable_name ( varmap_t* varmap, int id );

void help (int argc, char** argv) {
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\t%s hexastore/ -c\n", argv[0] );
	fprintf( stderr, "\t%s hexastore/ -p pred\n", argv[0] );
	fprintf( stderr, "\t%s hexastore/ -b subj pred obj\n", argv[0] );
	fprintf( stderr, "\t%s hexastore/ -j s1 p1 o1 [ s2 p2 o2 [ ... ] ]\n", argv[0] );
	fprintf( stderr, "\t%s hexastore/ subj pred obj\n", argv[0] );
	fprintf( stderr, "\n\n" );
}

int main (int argc, char** argv) {
	const char* dir	= NULL;
	char* arg		= NULL;
	
	if (argc < 2) {
		help(argc, argv);
		exit(1);
	}

	dir	= argv[1];
	if (argc > 2)
		arg		= argv[2];
	
	hx_storage_manager* st	= hx_new_memory_storage_manager();
	hx_hexastore* hx	= hx_open_tchexastore( st, dir );
	hx_nodemap* map		= hx_get_nodemap( hx );
	fprintf( stderr, "finished loading hexastore from file...\n" );
	
	if (arg == NULL) {
		hx_node* sn	= hx_new_variable( hx );
		hx_node* pn	= hx_new_variable( hx );
		hx_node* on	= hx_new_variable( hx );
		hx_variablebindings_iter* iter	= hx_get_statements_vb( hx, st, "subject", sn, "predicate", pn, "object", on, HX_SUBJECT, 0 );
		int subj_index	= hx_variablebindings_column_index( iter, "subject" );
		int pred_index	= hx_variablebindings_column_index( iter, "predicate" );
		int obj_index	= hx_variablebindings_column_index( iter, "object" );
		
		int count	= 1;
		while (!hx_variablebindings_iter_finished( iter )) {
			hx_node_id s, p, o;
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			s	= hx_variablebindings_node_id_for_binding( b, subj_index );
			p	= hx_variablebindings_node_id_for_binding( b, pred_index );
			o	= hx_variablebindings_node_id_for_binding( b, obj_index );
			print_triple( map, s, p, o, count++ );
			hx_variablebindings_iter_next( iter );
		}
		hx_free_variablebindings_iter( iter, 1 );
	} else if (strcmp( arg, "-c" ) == 0) {
		fprintf( stdout, "Triples: %lu\n", (unsigned long) hx_triples_count( hx, st ) );
// 	} else if (strcmp( arg, "-n" ) == 0) {
// 		// print out the nodemap
// 		hx_nodemap* map		= hx_get_nodemap( hx );
// 		size_t used	= avl_count( map->id2node );
// 		struct avl_traverser iter;
// 		avl_t_init( &iter, map->id2node );
// 		hx_nodemap_item* item;
// 		while ((item = (hx_nodemap_item*) avl_t_next( &iter )) != NULL) {
// 			char* string;
// 			hx_node_string( item->node, &string );
// 			fprintf( stdout, "%-10lu\t%s\n", (unsigned long) item->id, string );
// 			free( string );
// 		}
	} else if (strcmp( arg, "-id" ) == 0) {
		if (argc != 4) {
			help(argc, argv);
			exit(1);
		}
		char* uri	= argv[3];
		hx_node_id id	= node_id_for_string( uri, hx );
		fprintf( stdout, "<%s> == %d\n", uri, (int) id );
	} else if (strcmp( arg, "-p" ) == 0) {
		if (argc != 4) {
			help(argc, argv);
			exit(1);
		}
		char* pred	= argv[3];
		hx_node* node	= node_for_string( pred, hx );
		if (node != NULL) {
//			fprintf( stderr, "iter (*,%d,*) ordered by subject...\n", (int) id );
			hx_node* subj	= hx_new_variable( hx );
			hx_node* obj	= hx_new_variable( hx );
			hx_variablebindings_iter* iter	= hx_get_statements_vb( hx, st, "subject", subj, "predicate", node, "object", obj, HX_SUBJECT, 0 );
			int subj_index	= hx_variablebindings_column_index( iter, "subject" );
			int pred_index	= hx_variablebindings_column_index( iter, "predicate" );
			int obj_index	= hx_variablebindings_column_index( iter, "object" );
			
			int count	= 1;
			while (!hx_variablebindings_iter_finished( iter )) {
				hx_node_id s, p, o;
				hx_variablebindings* b;
				hx_variablebindings_iter_current( iter, &b );
				s	= hx_variablebindings_node_id_for_binding( b, subj_index );
				p	= hx_variablebindings_node_id_for_binding( b, pred_index );
				o	= hx_variablebindings_node_id_for_binding( b, obj_index );
				print_triple( map, s, p, o, count++ );
				hx_variablebindings_iter_next( iter );
			}
			hx_free_variablebindings_iter( iter, 1 );
		}
	} else if (strcmp( arg, "-b" ) == 0) {
		if (argc != 6) {
			help(argc, argv);
			exit(1);
		}
		char* subj	= argv[3];
		char* pred	= argv[4];
		char* obj	= argv[5];
		
		hx_node* sn	= node_for_string( subj, hx );
		hx_node* pn	= node_for_string( pred, hx );
		hx_node* on	= node_for_string( obj, hx );
		
		char* s	= (char*) ((sn == NULL) ? NULL : "subj");
		char* p	= (char*) ((pn == NULL) ? NULL : "pred");
		char* o	= (char*) ((on == NULL) ? NULL : "obj");
		hx_variablebindings_iter* iter	= hx_get_statements_vb( hx, st, s, sn, p, pn, o, on, HX_SUBJECT, 0 );
		int count	= 1;
		
		
		int size		= hx_variablebindings_iter_size( iter );
		char** names	= hx_variablebindings_iter_names( iter );
		while (!hx_variablebindings_iter_finished( iter )) {
			hx_variablebindings* b;
			hx_node_id s, p, o;
			hx_variablebindings_iter_current( iter, &b );
			char* string;
			hx_variablebindings_string( b, map, &string );
			fprintf( stdout, "%s\n", string );
			free( string );
			
			hx_free_variablebindings( b, 0 );
			hx_variablebindings_iter_next( iter );
		}
		hx_free_variablebindings_iter( iter, 1 );
	} else if (strcmp( arg, "-path" ) == 0) {
		char* from	= argv[3];
		char* to	= argv[4];
		
		hx_node* fnode	= node_for_string( from, hx );
		hx_node* knode	= node_for_string( "http://xmlns.com/foaf/0.1/knows", hx );
		hx_node* tnode	= node_for_string( to, hx );

		hx_node* v1		= hx_new_variable( hx );
		hx_node* v2		= hx_new_variable( hx );
		
		hx_variablebindings_iter* iter_a	= hx_get_statements_vb( hx, st, "from", fnode, NULL, knode, "friend", v1, HX_OBJECT, 0 );
		hx_variablebindings_iter* iter_b	= hx_get_statements_vb( hx, st, "friend", v2, NULL, knode, "to", tnode, HX_SUBJECT, 0 );
		
 		hx_variablebindings_iter* iter	= hx_new_mergejoin_iter( iter_a, iter_b );
		
		while (!hx_variablebindings_iter_finished( iter )) {
			hx_variablebindings* b;
			hx_node_id s, p, o;
			hx_variablebindings_iter_current( iter, &b );
			char* string;
			hx_variablebindings_string( b, map, &string );
			fprintf( stdout, "%s\n", string );
			free( string );
			
			hx_free_variablebindings( b, 0 );
			hx_variablebindings_iter_next( iter );
		}
		
		hx_free_variablebindings_iter( iter, 1 );
	} else if (strcmp( arg, "-star" ) == 0) {
		char* uri	= argv[3];
		hx_node* node	= node_for_string( uri, hx );
		
		hx_node* v1		= hx_new_variable( hx );
		hx_node* v2		= hx_new_variable( hx );
		hx_variablebindings_iter* iter_a	= hx_get_statements_vb( hx, st, "subj", node, "p1", v1, "o1", v2, HX_SUBJECT, 0 );
		hx_variablebindings_iter* iter_b	= hx_get_statements_vb( hx, st, "subj", node, "p2", v1, "o2", v2, HX_SUBJECT, 0 );
		hx_variablebindings_iter* iter	= hx_new_mergejoin_iter( iter_a, iter_b );
		
		while (!hx_variablebindings_iter_finished( iter )) {
			hx_variablebindings* b;
			hx_node_id s, p, o;
			hx_variablebindings_iter_current( iter, &b );
			char* string;
			hx_variablebindings_string( b, map, &string );
			fprintf( stdout, "%s\n", string );
			free( string );
			
			hx_free_variablebindings( b, 0 );
			hx_variablebindings_iter_next( iter );
		}
		
		hx_free_variablebindings_iter( iter, 1 );
	} else if (strcmp( arg, "-j" ) == 0) {
		if ((argc % 3) != 0) {
			help(argc, argv);
			exit(1);
		}
		
		varmap_t varmap;
		varmap.name2id	= avl_create( _varmap_name2id_cmp, NULL, &avl_allocator_default );
		varmap.id2name	= avl_create( _varmap_id2name_cmp, NULL, &avl_allocator_default );
		varmap.next	= 0;
		
		char* subj	= argv[3];
		char* pred	= argv[4];
		char* obj	= argv[5];
		hx_node* sn	= node_for_string_with_varmap( subj, hx, &varmap );
		hx_node* pn	= node_for_string_with_varmap( pred, hx, &varmap );
		hx_node* on	= node_for_string_with_varmap( obj, hx, &varmap );
		
		char* s	= (hx_node_is_variable( sn )) ? variable_name( &varmap, hx_node_iv( sn ) ) : NULL;
		char* p	= (hx_node_is_variable( pn )) ? variable_name( &varmap, hx_node_iv( pn ) ) : NULL;
		char* o	= (hx_node_is_variable( on )) ? variable_name( &varmap, hx_node_iv( on ) ) : NULL;
		hx_variablebindings_iter* iter	= hx_get_statements_vb( hx, st, s, sn, p, pn, o, on, HX_SUBJECT, 0 );
		
		fprintf( stderr, "constructed first variable bindings iterator...\n" );
		
		int index	= 6;
		int _count	= 2;
		while (index < argc) {
			char* subj	= argv[index++];
			char* pred	= argv[index++];
			char* obj	= argv[index++];
			hx_node* sn	= node_for_string_with_varmap( subj, hx, &varmap );
			hx_node* pn	= node_for_string_with_varmap( pred, hx, &varmap );
			hx_node* on	= node_for_string_with_varmap( obj, hx, &varmap );
			char* s	= (hx_node_is_variable( sn )) ? variable_name( &varmap, hx_node_iv( sn ) ) : NULL;
			char* p	= (hx_node_is_variable( pn )) ? variable_name( &varmap, hx_node_iv( pn ) ) : NULL;
			char* o	= (hx_node_is_variable( on )) ? variable_name( &varmap, hx_node_iv( on ) ) : NULL;
			hx_variablebindings_iter* _iter	= hx_get_statements_vb( hx, st, s, sn, p, pn, o, on, HX_SUBJECT, 0 );
//			hx_variablebindings_iter* _iter	= hx_new_index_iter_variablebindings( titer, st, s, p, o, 0 );
			fprintf( stderr, "constructed variable bindings iterator #%d... constructing mergejoin...\n", _count++ );
			hx_variablebindings_iter* join	= hx_new_mergejoin_iter( iter, _iter );
			iter	= join;
		}
		
		int size		= hx_variablebindings_iter_size( iter );
		char** names	= hx_variablebindings_iter_names( iter );
		while (!hx_variablebindings_iter_finished( iter )) {
			hx_variablebindings* b;
			hx_node_id s, p, o;
			hx_variablebindings_iter_current( iter, &b );
			char* string;
			hx_variablebindings_string( b, map, &string );
			fprintf( stdout, "%s\n", string );
			free( string );
			
			hx_free_variablebindings( b, 0 );
			hx_variablebindings_iter_next( iter );
		}
		hx_free_variablebindings_iter( iter, 1 );
	} else {
		if (argc != 5) {
			help(argc, argv);
			exit(1);
		}
		char* subj	= arg;
		char* pred	= argv[3];
		char* obj	= argv[4];
		
		hx_node* sn	= node_for_string( subj, hx );
		hx_node* pn	= node_for_string( pred, hx );
		hx_node* on	= node_for_string( obj, hx );
//		fprintf( stderr, "iter (%d,%d,%d) ordered by subject...\n", (int) sid, (int) pid, (int) oid );
		hx_variablebindings_iter* iter	= hx_get_statements_vb( hx, st, "subject", sn, "predicate", pn, "object", on, HX_SUBJECT, 0 );
		
		int subj_index	= hx_variablebindings_column_index( iter, "subject" );
		int pred_index	= hx_variablebindings_column_index( iter, "predicate" );
		int obj_index	= hx_variablebindings_column_index( iter, "object" );
		
		int count	= 1;
		while (!hx_variablebindings_iter_finished( iter )) {
			hx_node_id s, p, o;
			hx_variablebindings* b;
			hx_variablebindings_iter_current( iter, &b );
			s	= hx_variablebindings_node_id_for_binding( b, subj_index );
			p	= hx_variablebindings_node_id_for_binding( b, pred_index );
			o	= hx_variablebindings_node_id_for_binding( b, obj_index );
			print_triple( map, s, p, o, count++ );
			hx_variablebindings_iter_next( iter );
		}
		hx_free_variablebindings_iter( iter, 1 );
	}
	
	hx_free_hexastore( hx, st );
	hx_free_storage_manager( st );
	return 0;
}

void print_triple ( hx_nodemap* map, hx_node_id s, hx_node_id p, hx_node_id o, int count ) {
// 	fprintf( stderr, "[%d] %d, %d, %d\n", count++, (int) s, (int) p, (int) o );
	hx_node* sn	= hx_nodemap_get_node( map, s );
	hx_node* pn	= hx_nodemap_get_node( map, p );
	hx_node* on	= hx_nodemap_get_node( map, o );
	char *ss, *sp, *so;
	hx_node_string( sn, &ss );
	hx_node_string( pn, &sp );
	hx_node_string( on, &so );
	if (count > 0) {
		fprintf( stdout, "[%d] ", count );
	}
	fprintf( stdout, "%s %s %s\n", ss, sp, so );
	free( ss );
	free( sp );
	free( so );
}

hx_node_id node_id_for_string ( char* string, hx_hexastore* hx ) {
	hx_nodemap* map	= hx_get_nodemap( hx );
	static int var_id	= -100;
	hx_node_id id;
	hx_node* node;
	if (strcmp( string, "-" ) == 0) {
		id	= (hx_node_id) var_id--;
	} else if (strcmp( string, "0" ) == 0) {
		id	= (hx_node_id) 0;
	} else if (*string == '-') {
		id	= 0 - atoi( string+1 );
	} else {
		node	= hx_new_node_resource( string );
		id		= hx_nodemap_get_node_id( map, node );
		hx_free_node( node );
		if (id <= 0) {
			fprintf( stderr, "No such subject found: '%s'.\n", string );
		}
	}
	return id;
}

hx_node* node_for_string ( char* string, hx_hexastore* hx ) {
	if (strcmp( string, "-" ) == 0) {
		return hx_new_variable( hx );
	} else if (strcmp( string, "0" ) == 0) {
		return NULL;
	} else if (*string == '-') {
		return hx_new_node_variable( 0 - atoi( string+1 ) );
	} else {
		return hx_new_node_resource( string );
	}
}

hx_node* node_for_string_with_varmap ( char* string, hx_hexastore* hx, varmap_t* varmap ) {
	if (strcmp( string, "-" ) == 0) {
		return hx_new_variable( hx );
	} else if (string[0] == '?') {
		varmap_item_t* item	= (varmap_item_t*) avl_find( varmap->name2id, &( string[1] ) );
		if (item == NULL) {
			item	= (varmap_item_t*) calloc( 1, sizeof(varmap_item_t) );
			item->name	= &( string[1] );
			item->id		= --( varmap->next );
			avl_insert( varmap->name2id, item );
			avl_insert( varmap->id2name, item );
		}
		return hx_new_node_variable( item->id );
	} else if (strcmp( string, "0" ) == 0) {
		return NULL;
	} else if (*string == '-') {
		return hx_new_node_variable( 0 - atoi( string+1 ) );
	} else {
		return hx_new_node_resource( string );
	}
}

char* variable_name ( varmap_t* varmap, int id ) {
	varmap_item_t i;
	i.id	= id;
	varmap_item_t* item	= (varmap_item_t*) avl_find( varmap->id2name, &i );
	if (item != NULL) {
		return item->name;
	} else {
		return NULL;
	}
}
