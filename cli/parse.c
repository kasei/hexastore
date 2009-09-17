#include <time.h>
#include <stdio.h>
#include <raptor.h>
#include <inttypes.h>
#include "mentok.h"
#include "rdf/node.h"
#include "parser/parser.h"
#include "store/hexastore/hexastore.h"
#include "store/tokyocabinet/tokyocabinet.h"

// #define DIFFTIME(a,b) ((b-a)/(double)CLOCKS_PER_SEC)
#define DIFFTIME(a,b) (b-a)

void help (int argc, char** argv);
int main (int argc, char** argv);

static int count	= 0;

void help (int argc, char** argv) {
	fprintf( stderr, "Usage: %s -store=S data.rdf data/\n\n", argv[0] );
	fprintf( stderr, "S must be one of the following:\n" );
	fprintf( stderr, "    'T' - Use the tokyocabinet backend with files stored in the directory data/\n" );
	fprintf( stderr, "    'H' - Use the hexastore memory backend serialized to the file data.\n\n" );
}

void logger ( uint64_t _count, void* thunk ) {
	time_t* c	= (time_t*) thunk;
	time_t end_time	= time(NULL);
	double elapsed	= DIFFTIME(*c, end_time);
	double tps	= ((double) _count / elapsed);
	fprintf( stderr, "\rParsed %lu triples in %.1lf seconds (%.1lf triples/second)...", (unsigned long) _count, elapsed, tps );
}

int main (int argc, char** argv) {
	const char* rdf_filename	= NULL;
	const char* output_location	= NULL;
	
	if (argc < 4) {
		help(argc, argv);
		exit(1);
	}
	
	char type	= 'T';
	int i	= 1;
	if (strncmp(argv[i], "-store=", 7) == 0) {
		switch (argv[i][7]) {
			case 'T':
				type	= 'T';
				break;
			case 'H':
				type	= 'H';
				break;
			default:
				fprintf( stderr, "Unrecognized store type.\n\n" );
				exit(1);
		};
		i++;
	} else {
		fprintf( stderr, "No store type specified.\n" );
		exit(1);
	}
	
	rdf_filename	= argv[i++];
	output_location	= argv[i++];
	
	const char* index_string;
	if (i < argc) {
		index_string	= argv[i++];
	} else {
		index_string	= "spo,sop,pso,pos,osp,ops";
	}
	
	hx_model* hx;
	if (type == 'T') {
		hx_store* store	= hx_new_store_tokyocabinet( NULL, output_location );
		hx				= hx_new_hexastore_with_store( NULL, store );
	} else {
		hx_store* store	= hx_new_store_hexastore_with_indexes( NULL, index_string );
		hx				= hx_new_hexastore_with_store( NULL, store );
	}
	
	time_t st_time;
	hx_parser* parser	= hx_new_parser();
	hx_parser_set_logger( parser, logger, &st_time );
	
	st_time	= time(NULL);
	hx_store_begin_bulk_load( hx->store );
	uint64_t total	= hx_parser_parse_file_into_hexastore( parser, hx, rdf_filename );
	hx_store_end_bulk_load( hx->store );
	time_t end_time	= time(NULL);
	
	double elapsed	= DIFFTIME(st_time, end_time);
	double tps	= ((double) total / elapsed);
	fprintf( stderr, "\rParsed %lu triples in %.1lf seconds (%.1lf triples/second)\n", (unsigned long) total, elapsed, tps );
	
	if (type == 'H') {
		FILE* f = NULL;
		if (strcmp(output_location, "/dev/null") != 0) {
			f	= fopen( output_location, "w" );
			if (f == NULL) {
				perror( "Failed to open hexastore file for writing" );
				return 1;
			}
		}
		if (hx_store_hexastore_write( hx->store, f ) != 0) {
			fprintf( stderr, "*** Couldn't write hexastore to disk.\n" );
			return 1;
		}
	}
	
	hx_free_parser( parser );
	hx_free_hexastore( hx );

	time_t finalize_time	= time(NULL);
	double felapsed	= DIFFTIME(st_time, finalize_time);
	double ftps	= ((double) total / felapsed);
	fprintf( stderr, "\rFinalized at %.1lf seconds (%.1lf triples/second)\n", felapsed, ftps );
	
	return 0;
}
