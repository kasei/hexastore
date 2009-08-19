#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <raptor.h>
#include <inttypes.h>
#include <tcutil.h>
#include <tcbdb.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "hexastore.h"
#include "node.h"

typedef struct {
	FILE* f;
	TCBDB* cabinet;
	uint64_t count;
	uint64_t next_bnode;
	uint64_t next_id;
	struct timeval tv;
} parser_t;

void help (int argc, char** argv);
int main (int argc, char** argv);

void help (int argc, char** argv) {
	fprintf( stderr, "Usage: %s nodemap.tcb data.rdf triples.data\n\n", argv[0] );
}

void logger ( uint64_t _count ) {
	fprintf( stderr, "\rParsed %lu triples...", (unsigned long) _count );
}

unsigned char* generate_id_handler (void *user_data, raptor_genid_type type, unsigned char* user_bnodeid) {
	parser_t* parser	= (parser_t*) user_data;
	
	uint64_t seconds	= 0;
	uint64_t copy		= seconds;
	static char encodingTable [64] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W',    'Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3','4','5','6','7','8','9','_','.','-'
	};
	
	unsigned char* id	= (unsigned char*) calloc( 48, sizeof( char ) );
	unsigned char* p	= id;
	*(p++)	= 'X';
	while (copy != 0) {
		int i	= (int) (copy & 0x3f);
		*(p++)	= encodingTable[ i ];
		copy >>= 4;
	}
	*(p++)	= 'X';
	copy	= 0;
	while (copy != 0) {
		int i	= (int) (copy & 0x3f);
		*(p++)	= encodingTable[ i ];
		copy >>= 4;
	}
	*(p++)	= 'X';
	copy	= parser->next_bnode++;
	while (copy != 0) {
		int i	= (int) (copy & 0x3f);
		*(p++)	= encodingTable[ i ];
		copy >>= 4;
	}
	*(p++)	= (char) 0;
	if (user_bnodeid != NULL) {
		free( user_bnodeid );
	}
	return id;
}

hx_node_id parser_node ( parser_t* parser, void* node, raptor_identifier_type type, char* lang, raptor_uri* dt ) {
	hx_node_id id	= 0;
	char node_type;
	char* value;
	int needs_free	= 0;
	char* language	= NULL;
	char* datatype	= NULL;
	hx_node* newnode;
	
	switch (type) {
		case RAPTOR_IDENTIFIER_TYPE_RESOURCE:
		case RAPTOR_IDENTIFIER_TYPE_PREDICATE:
			value		= (char*) raptor_uri_as_string((raptor_uri*)node);
			newnode		= hx_new_node_resource( value );
			node_type	= 'R';
			break;
		case RAPTOR_IDENTIFIER_TYPE_ANONYMOUS:
			value		= (char*) node;
			newnode		= hx_new_node_blank( value );
			node_type	= 'B';
			break;
		case RAPTOR_IDENTIFIER_TYPE_LITERAL:
			value		= (char*)node;
			node_type	= 'L';
			if(lang && type == RAPTOR_IDENTIFIER_TYPE_LITERAL) {
				language	= (char*) lang;
				newnode		= (hx_node*) hx_new_node_lang_literal( value, language );
			} else if (dt) {
				datatype	= (char*) raptor_uri_as_string((raptor_uri*) dt);
				newnode		= (hx_node*) hx_new_node_dt_literal( value, datatype );
			} else {
				newnode		= hx_new_node_literal( value );
			}
			break;
		case RAPTOR_IDENTIFIER_TYPE_XML_LITERAL:
			value		= (char*) node;
			datatype	= "http://www.w3.org/1999/02/22-rdf-syntax-ns#XMLLiteral";
			node_type	= 'L';
			newnode		= (hx_node*) hx_new_node_dt_literal( value, datatype );
			break;
		case RAPTOR_IDENTIFIER_TYPE_ORDINAL:
			needs_free	= 1;
			value		= (char*) malloc( 64 );
			if (value == NULL) {
				fprintf( stderr, "*** malloc failed in _hx_parser_node\n" );
			}
			sprintf( value, "http://www.w3.org/1999/02/22-rdf-syntax-ns#_%d", *((int*) node) );
			newnode		= hx_new_node_resource( value );
			node_type	= 'R';
			break;
		case RAPTOR_IDENTIFIER_TYPE_UNKNOWN:
		default:
			fprintf(stderr, "*** unknown node type %d\n", type);
			return 0;
	}
	
	char* string;
	hx_node_string( newnode, &string );
	
	if (tcbdbputkeep(parser->cabinet, string, strlen(string), &(parser->next_id), sizeof(uint64_t))) {
		parser->next_id++;
//		fprintf(stderr, "adding: %s\n", string);
	} else {
//		fprintf(stderr, "skipping: %s\n", string);
	}
	
	int len;
	void* p		= tcbdbget(parser->cabinet, string, strlen(string), &len);
	id			= *((hx_node_id*) p);
	free(string);
	
	if (needs_free) {
		free( value );
		needs_free	= 0;
	}
	hx_free_node( newnode );
	return id;
}

int get_triple_nodes ( parser_t* parser, const raptor_statement* triple, hx_node_id* s, hx_node_id* p, hx_node_id* o ) {
	*s	= parser_node( parser, (void*) triple->subject, triple->subject_type, NULL, NULL );
	*p	= parser_node( parser, (void*) triple->predicate, triple->predicate_type, NULL, NULL );
	*o	= parser_node( parser, (void*) triple->object, triple->object_type, (char*) triple->object_literal_language, triple->object_literal_datatype );
	return 0;
}

void statement_handler (void* user_data, const raptor_statement* triple)	{
	parser_t* parser	= (parser_t*) user_data;
	hx_node_id s, p, o;
	
	get_triple_nodes( parser, triple, &s, &p, &o );
	
	fwrite( &s, sizeof( hx_node_id ), 1, parser->f );
	fwrite( &p, sizeof( hx_node_id ), 1, parser->f );
	fwrite( &o, sizeof( hx_node_id ), 1, parser->f );
	
	int i	= parser->count++;
	if (i % 25000 == 0) {
		logger( i );
	}
}

int main (int argc, char** argv) {
	int ecode;
	const char* rdf_filename		= NULL;
	const char* nodemap_filename	= NULL;
	const char* triples_filename	= NULL;
	
	if (argc < 2) {
		help(argc, argv);
		exit(1);
	}
	
	nodemap_filename	= argv[1];
	parser_t* parser	= (parser_t*) calloc( 1, sizeof( parser_t ) );
	parser->count		= 0;
	parser->next_id		= 1;
	parser->next_bnode	= 0;
	gettimeofday( &( parser->tv ), NULL );
	parser->cabinet	= tcbdbnew();

	if (argc >= 4) {
		tcbdbsetcache(parser->cabinet, 1024*1024, 1024*1024);
	}

	if(!tcbdbopen(parser->cabinet, nodemap_filename, HDBOWRITER | HDBOCREAT)) {
		ecode = tcbdbecode(parser->cabinet);
		fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
		exit(1);
	}
	tcbdboptimize(parser->cabinet, 0, 0, 0, -1, -1, BDBTLARGE|BDBTDEFLATE);
	
	int len;
	void* p;
	if (argc >= 4) {
		rdf_filename		= argv[2];
		triples_filename	= argv[3];
		parser->f			= fopen( triples_filename, "a" );
		
		raptor_init();
		unsigned char* uri_string	= raptor_uri_filename_to_uri_string( rdf_filename );
		raptor_uri* uri				= raptor_new_uri(uri_string);
		const char* parser_name		= raptor_guess_parser_name(NULL, NULL, NULL, 0, uri_string);
		raptor_parser* rdf_parser	= raptor_new_parser( parser_name );
		raptor_uri *base_uri		= raptor_uri_copy(uri);
			
		if ((p = tcbdbget(parser->cabinet, "   NEXTID", 9, &len))) {
			uint64_t* ptr	= (uint64_t*) p;
			parser->next_id	= *ptr;
		}
		
		raptor_set_statement_handler(rdf_parser, parser, statement_handler);
//		raptor_set_generate_id_handler(rdf_parser, parser, generate_id_handler);
		raptor_parse_file(rdf_parser, uri, base_uri);
		logger( parser->count );
		fprintf( stderr, "\n" );
		tcbdbput(parser->cabinet, "   NEXTID", 9, &( parser->next_id ), sizeof( uint64_t ));
		
		fclose( parser->f );
		raptor_free_parser(rdf_parser);
		free( uri_string );
		free( base_uri );
		free( uri );
	} else {
		BDBCUR *cur	= tcbdbcurnew(parser->cabinet);
		tcbdbcurfirst(cur);
		while((p = (void*) tcbdbcurkey3(cur, &len)) != NULL) {
			uint64_t* idp;
			int idlen;
			idp	= (uint64_t*) tcbdbcurval3(cur, &idlen);
			printf("%10"PRIu64"\t%s\n", *idp, (char*) p);
			tcbdbcurnext(cur);
		}
		tcbdbcurdel(cur);
	}
	
	if(!tcbdbclose(parser->cabinet)) {
		ecode = tcbdbecode(parser->cabinet);
		fprintf(stderr, "close error: %s\n", tcbdberrmsg(ecode));
		exit(1);
	}
	
	return 0;
}
