#include "rasqal_parser.h"
#include "hexastore.h"
#include "graphpattern.h"

typedef struct {
	char* name;
	int id;
	void* next;
} hx_sparqlparser_variable_map_list;

void roqet_write_indent(FILE *fh, int indent);
hx_graphpattern* hx_parser_graph_pattern_walk (rasqal_graph_pattern *gp, int gp_index, FILE *fh, hx_sparqlparser_variable_map_list* vmap, int indent);
hx_graphpattern* hx_parser_query_walk (rasqal_query *rq, FILE *fh, int indent);

int _hx_parser_variable_id_with_name ( hx_sparqlparser_variable_map_list* vmap, char* name ) {
	int current_id	= 0;
	
	hx_sparqlparser_variable_map_list* p	= vmap;
	hx_sparqlparser_variable_map_list* last	= p;
	while (p != NULL) {
		current_id	= p->id;
		char* n		= p->name;
		if (n && strcmp(n,name) == 0) {
//			fprintf( stderr, "variable '%s' has id %d\n", name, p->id );
			return p->id;
		}
		last	= p;
		p		= (hx_sparqlparser_variable_map_list*) p->next;
	}
	
	--current_id;
	
	hx_sparqlparser_variable_map_list* new	= (hx_sparqlparser_variable_map_list*) calloc( 1, sizeof( hx_sparqlparser_variable_map_list ) );
	new->name	= name;
	new->id		= current_id;
	new->next	= NULL;
	last->next	= new;

//	fprintf( stderr, "variable '%s' has id %d\n", name, current_id );
	return current_id;
}

void _hx_parser_free_vmap ( hx_sparqlparser_variable_map_list* vmap ) {
	if (vmap->next) {
		_hx_parser_free_vmap(vmap->next);
	}
	free(vmap);
}

void roqet_write_indent(FILE *fh, int indent) {
	while(indent > 0) {
		int sp =(indent > SPACES_LENGTH) ? SPACES_LENGTH : indent;
		(void)fwrite(spaces, sizeof(char), sp, fh);
		indent -= sp;
	}
}

hx_node* _hx_parser_create_node ( rasqal_literal* l, hx_sparqlparser_variable_map_list* vmap ) {
	FILE* fh	= stderr;
	int vid;
	rasqal_variable* v;
	hx_node* n	= NULL;
	switch(l->type) {
		case RASQAL_LITERAL_URI:
			n	= hx_new_node_resource( (char*) raptor_uri_as_string(l->value.uri) );
			break;
		case RASQAL_LITERAL_BLANK:
			fprintf(stderr, "*** BLANK in query parse: %s", l->string);
			break;
		case RASQAL_LITERAL_PATTERN:
			fprintf(stderr, "*** PATTERN in query parse: /%s/%s", l->string, l->flags ? (const char*)l->flags : "");
			break;
		case RASQAL_LITERAL_STRING:
			if(l->language) {
				n	= (hx_node*) hx_new_node_lang_literal( (char*) l->string, (char*) l->language );
			} else if(l->datatype) {
				n	= (hx_node*) hx_new_node_dt_literal( (char*) l->string, (char*) raptor_uri_as_string(l->datatype) );
			} else {
				n	= hx_new_node_literal( (char*) l->string );
			}
			break;
		case RASQAL_LITERAL_VARIABLE:
			v	= l->value.variable;
			vid	= _hx_parser_variable_id_with_name( vmap, (char*) v->name );
			if (v->type == RASQAL_VARIABLE_TYPE_ANONYMOUS) {
				n	= hx_new_node_named_variable_nondistinguished( vid, (char*) v->name );
			} else {
				n	= hx_new_node_named_variable( vid, (char*) v->name );
			}
			break;

		case RASQAL_LITERAL_QNAME:
		case RASQAL_LITERAL_INTEGER:
			n	= (hx_node*) hx_new_node_dt_literal( (char*) l->string, "http://www.w3.org/2001/XMLSchema#integer" );
			break;
		case RASQAL_LITERAL_BOOLEAN:
			n	= (hx_node*) hx_new_node_dt_literal( (char*) l->string, "http://www.w3.org/2001/XMLSchema#boolean" );
			break;
		case RASQAL_LITERAL_DOUBLE:
			n	= (hx_node*) hx_new_node_dt_literal( (char*) l->string, "http://www.w3.org/2001/XMLSchema#double" );
			break;
		case RASQAL_LITERAL_FLOAT:
			n	= (hx_node*) hx_new_node_dt_literal( (char*) l->string, "http://www.w3.org/2001/XMLSchema#float" );
			break;
		case RASQAL_LITERAL_DECIMAL:
			n	= (hx_node*) hx_new_node_dt_literal( (char*) l->string, "http://www.w3.org/2001/XMLSchema#decimal" );
			break;
		case RASQAL_LITERAL_DATETIME:
			n	= (hx_node*) hx_new_node_dt_literal( (char*) l->string, "http://www.w3.org/2001/XMLSchema#dateTime" );
			break;

		case RASQAL_LITERAL_UNKNOWN:
		default:
			fprintf(stderr, "Unknown literal type %d", l->type);
	}
	return n;
}

hx_graphpattern* hx_parser_graph_pattern_walk (rasqal_graph_pattern *gp, int gp_index, FILE *fh, hx_sparqlparser_variable_map_list* vmap, int indent) {
	
	int idx;
	int triple_index=0;
	raptor_sequence *seq;
	
	rasqal_graph_pattern_operator op	= rasqal_graph_pattern_get_operator(gp);
	fprintf( stderr, "> hx_parser_graph_pattern_walk (%d)\n", op );
	
	if (1) {
		roqet_write_indent(fh, indent);
		fprintf(fh, ">>%s<< graph pattern", rasqal_graph_pattern_operator_as_string(op));
		idx=rasqal_graph_pattern_get_index(gp);
		if (idx >= 0)
			fprintf(fh, "[%d]", idx);
		if (gp_index >= 0)
			fprintf(fh, " #%d", gp_index);
		fputs(" {\n", fh);
		indent+= 1;
	}

	fprintf( stderr, "+1 hx_parser_graph_pattern_walk (%d)\n", op );
	
	/* look for triples */
	hx_container_t* triples	= hx_new_container('T', 4 );
	while (1) {
		rasqal_triple* t	= rasqal_graph_pattern_get_triple(gp, triple_index);
		if (!t)
			break;
		hx_node* s		= _hx_parser_create_node( t->subject, vmap );
		hx_node* p		= _hx_parser_create_node( t->predicate, vmap );
		hx_node* o		= _hx_parser_create_node( t->object, vmap );
		
		if (t->origin) {
			hx_node* g	= _hx_parser_create_node( t->origin, vmap );
			// XXX
		}
		
		hx_triple* hxt	= hx_new_triple( s, p, o );
		hx_container_push_item( triples, hxt );
		triple_index++;
	}

	hx_graphpattern* pat	= NULL;
	
	int tsize				= hx_container_size(triples);
	if (tsize > 0) {
		hx_bgp* b	= hx_new_bgp( tsize, (hx_triple**) triples->items );
		pat	= hx_new_graphpattern( HX_GRAPHPATTERN_BGP, b );
	}

	fprintf( stderr, "+2 hx_parser_graph_pattern_walk (%d)\n", op );
	
// 	int i;
// 	for (i = 0; i < tsize; i++) {
// 		int j;
// 		hx_triple* t	= hx_container_item(triples,i);
// 		for (j = 0; j < 3; j++) {
// 			hx_node* n	= hx_triple_node( t, j );
// 			hx_free_node(n);
// 		}
// 		hx_free_triple(t);
// 	}
	hx_free_container(triples);
	
	
	
	/* look for constraints */
	seq=rasqal_graph_pattern_get_constraint_sequence(gp);
	if (seq && raptor_sequence_size(seq) > 0) {
		roqet_write_indent(fh, indent);
// 		fprintf(fh, "constraints (%d) {\n", raptor_sequence_size(seq));

		gp_index=0;
		while(1) {
			rasqal_expression* expr=rasqal_graph_pattern_get_constraint(gp, gp_index);
			if (!expr)
				break;
			
			roqet_write_indent(fh, indent+2);
// 			fprintf(fh, "constraint #%d { ", gp_index);
// 			rasqal_expression_print(expr, fh);
// 			fputs("}\n", fh);
			
			gp_index++;
		}

	fprintf( stderr, "+3 hx_parser_graph_pattern_walk (%d)\n", op );

// 		roqet_write_indent(fh, indent);
// 		fputs("}\n", fh);
	}
	
	// basic graph patterns don't have sub-patterns, so return right now.
	if (op == RASQAL_GRAPH_PATTERN_OPERATOR_BASIC) {
		return pat;
	}
	
	/* look for sub-graph patterns */
	hx_container_t* subgps	= hx_new_container('G', 4 );
	seq=rasqal_graph_pattern_get_sub_graph_pattern_sequence(gp);
	if (seq && raptor_sequence_size(seq) > 0) {
		if (0) {
			roqet_write_indent(fh, indent);
			fprintf(fh, "sub-graph patterns (%d) {\n", raptor_sequence_size(seq));
		}

		gp_index=0;
		while(1) {
			rasqal_graph_pattern* sgp;
			sgp=rasqal_graph_pattern_get_sub_graph_pattern(gp, gp_index);
			if (!sgp)
				break;
			
			hx_graphpattern* g	= hx_parser_graph_pattern_walk(sgp, gp_index, fh, vmap, indent+2);
			if (g) {
				hx_container_push_item( subgps, g );
				
				char* string;
				hx_graphpattern_sse( g, &string, "  ", 0 );
				fprintf( stderr, "PARSED SUB-GRAPH PATTERN in (%s): %s\n", rasqal_graph_pattern_operator_as_string(op), string );
				free( string );
			}
			gp_index++;
		}
		
		if (0) {
			roqet_write_indent(fh, indent);
			fputs("}\n", fh);
		}
	}
	
	fprintf( stderr, "+4 hx_parser_graph_pattern_walk (%d)\n", op );

	int gsize				= hx_container_size(subgps);
	char* opn	= rasqal_graph_pattern_operator_as_string(op);
	fprintf( stderr, "handling sub-graph patterns for %s\n", opn );
	if (gsize > 0) {
		if (strcmp(opn,"group") == 0) {
			fprintf( stderr, "got a RASQAL_GRAPH_PATTERN_OPERATOR_GROUP\n" );
			pat	= hx_new_graphpattern_ptr( HX_GRAPHPATTERN_GROUP, gsize, subgps->items );
		} else {
			pat	= NULL;
			fprintf( stderr, "unimplemented graph pattern operator %d '%s' in parser\n", op, rasqal_graph_pattern_operator_as_string(op) );
			fprintf( stderr, "GROUP op is %d\n", RASQAL_GRAPH_PATTERN_OPERATOR_GROUP );
		}
	} else if (pat) {
		fprintf( stderr, "no sub graph patterns, but there's a BGP.\n" );
	}
	
	if (1) {
		indent-=2;
		roqet_write_indent(fh, indent);
		fputs("}\n", fh);
	}
	
	return pat;
}


		
hx_graphpattern* hx_parser_query_walk (rasqal_query *rq, FILE *fh, int indent) {
	rasqal_query_verb verb;
	int i;
	rasqal_graph_pattern* gp;
	raptor_sequence *seq;

	hx_sparqlparser_variable_map_list* vmap	= (hx_sparqlparser_variable_map_list*) calloc( 1, sizeof(hx_sparqlparser_variable_map_list) );
	vmap->id	= 0;
	vmap->name	= NULL;
	vmap->next	= NULL;

	verb	= rasqal_query_get_verb(rq);
	roqet_write_indent(fh, indent);
	fprintf(fh, "query verb: %s\n", rasqal_query_verb_as_string(verb));

	i	= rasqal_query_get_distinct(rq);
	if (i != 0) {
		roqet_write_indent(fh, indent);
		fprintf(fh, "query asks for distinct results\n");
	}
	
	i	= rasqal_query_get_limit(rq);
	if (i >= 0) {
		roqet_write_indent(fh, indent);
		fprintf(fh, "query asks for result limits %d\n", i);
	}
	
	i=rasqal_query_get_offset(rq);
	if (i >= 0) {
		roqet_write_indent(fh, indent);
		fprintf(fh, "query asks for result offset %d\n", i);
	}
	
	seq	= rasqal_query_get_bound_variable_sequence(rq);
	int project_vars_count	= raptor_sequence_size(seq);
	fprintf(fh, "%d project variables:\n", project_vars_count);
	hx_node** project_vars	= (hx_node**) calloc( project_vars_count, sizeof( hx_node* ) );
	int pindex	= 0;
	if (seq && raptor_sequence_size(seq) > 0) {
		fprintf(fh, "query bound variables (%d): ", raptor_sequence_size(seq));
		i=0;
		while (1) {
			rasqal_variable* v=(rasqal_variable*)raptor_sequence_get_at(seq, i);
			if (!v)
				break;

// 			if (i > 0)
// 				fputs(", ", fh);
			
			int id	= _hx_parser_variable_id_with_name( vmap, (char*) v->name );
//			project_vars[ pindex++ ]	= hx_new_node_named_variable
			fputs((const char*)v->name, fh);
// 			if (v->expression) {
// 				fputc('=', fh);
// 				rasqal_expression_print(v->expression, fh);
// 			}
			
			
			
			i++;
		}
		fputc('\n', fh);
	}

	gp=rasqal_query_get_query_graph_pattern(rq);
	if (!gp)
		return NULL;


	seq=rasqal_query_get_construct_triples_sequence(rq);
	if (seq && raptor_sequence_size(seq) > 0) {
		roqet_write_indent(fh, indent);
		fprintf(fh, "query construct triples (%d) {\n", 
						raptor_sequence_size(seq));
		i=0;
		while(1) {
			rasqal_triple* t=rasqal_query_get_construct_triple(rq, i);
			if (!t)
				break;
		
			roqet_write_indent(fh, indent+2);
			fprintf(fh, "triple #%d { ", i);
			rasqal_triple_print(t, fh);
			fputs(" }\n", fh);

			i++;
		}
		roqet_write_indent(fh, indent);
		fputs("}\n", fh);
	}

	fputs("query ", fh);
	hx_graphpattern* g	= hx_parser_graph_pattern_walk(gp, -1, fh, vmap, indent);
	_hx_parser_free_vmap( vmap );
	return g;
}

void* hx_parse_query ( unsigned char* query_string, unsigned char* base_uri_string ) {
	unsigned char *uri_string		= NULL;
	int free_uri_string				= 0;
	const char *ql_name				= "sparql";
	char *ql_uri					= NULL;
	rasqal_query *rq;
	int rc							= 0;
	raptor_uri *uri					= NULL;
	raptor_uri *base_uri			= NULL;
	rasqal_query_results_formatter* results_formatter = NULL;
	
	void* world=rasqal_new_world();
	if (!world) {
		fprintf(stderr, "rasqal_world init failed\n");
		return NULL;
	}
	
	fprintf(stderr, "Running query '%s' with base URI %s\n",
					(char*)query_string, base_uri_string);
	
	rq	= rasqal_new_query(world, (const char*)ql_name, (const unsigned char*)ql_uri);

	if (rasqal_query_prepare(rq, (const unsigned char*)query_string, base_uri)) {
		size_t len	= strlen((const char*)query_string);
		
		fprintf(stderr, "Parsing query '");
		(void)fwrite(query_string, len, sizeof(char), stderr);
		fputs("' failed\n", stderr);
		rc=1;
		goto tidy_query;
	}

	hx_graphpattern* g	= hx_parser_query_walk(rq, stdout, 0);
	if (g) {
		char* string;
		hx_graphpattern_sse( g, &string, "  ", 0 );
		fprintf( stderr, "PARSED QUERY: %s\n", string );
		free( string );
	}


	
tidy_query:	
	if (results_formatter)
		rasqal_free_query_results_formatter(results_formatter);

	rasqal_free_query(rq);

	if (base_uri)
		raptor_free_uri(base_uri);
	if (uri)
		raptor_free_uri(uri);
	if (free_uri_string)
		raptor_free_memory(uri_string);

	rasqal_free_world(world);
	
	if (rc) {
		return NULL;
	} else {
		return NULL;
	}
}
