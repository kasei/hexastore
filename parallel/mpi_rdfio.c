#include "parallel/mpi_rdfio.h"
#include "parallel/mpi_file_ntriples_node_iterator.h"
#include "parallel/async_des.h"
#include "genmap/avl_tree_map.h"
#include "parallel/parallel.h"
#include "misc/util.h"

typedef struct {
	hx_node_id gid;
	int lid;
	int pid;
} _mpi_rdfio_lookup_record;

int _mpi_rdfio_compare_string(const void*, const void*, void*);
int _mpi_rdfio_compare_hx_node_id(const void*, const void*, void*);
void _mpi_rdfio_destroy_entry_with_free(void*, void*, void*);

int _mpi_rdfio_send_lookup(async_mpi_session*, void*);
int _mpi_rdfio_recv_lookup(async_mpi_session*, void*);

int _mpi_rdfio_send_answer(async_mpi_session*, void*);
int _mpi_rdfio_recv_answer(async_mpi_session*, void*);

hx_node* _mpi_rdfio_to_hx_node_p(char* ntnode);

// #define MPI_RDFIO_DEBUG(s, ...) fprintf(stderr, "%s:%u: "s"", __FILE__, __LINE__, __VA_ARGS__)
#define MPI_RDFIO_DEBUG(s, ...)

int mpi_rdfio_readnt(char *filename, char *mapfilename, size_t bufsize, hx_model **store, MPI_Comm comm) {
	MPI_File file;
	MPI_Offset filesize;
	MPI_Info info;
	int rank, commsize;

	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &commsize);
	
	MPI_RDFIO_DEBUG("%i: Creating store if %p == %p.\n", rank, *store, NULL);

	if(*store == NULL) {
		*store = hx_new_hexastore(NULL);
		if(*store == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot allocate hexastore.\n", __FILE__, __LINE__);
			return -1;
		}
	}

	MPI_RDFIO_DEBUG("%i: Opening file %s.\n", rank, filename);

	MPI_Info_create(&info);
	
	MPI_File_open(comm, filename, MPI_MODE_RDONLY, info, &file);

	MPI_File_get_size(file, &filesize);
	MPI_Offset chunk = filesize / commsize;
	MPI_Offset my_chunk = chunk;
	if(rank == commsize - 1) {
		my_chunk += filesize % commsize;
	}
	
	hx_node * nodes[3];
	int nodecnt = 0;
	
	MPI_RDFIO_DEBUG("%i: Loading triples from %llu bytes located at offset %llu bytes in a file of total size %llu bytes.\n", rank, my_chunk, rank*chunk, filesize);
	
	iterator_t iter = mpi_file_ntriples_node_iterator_create(file, rank*chunk, my_chunk, bufsize, comm);
	
	while(iterator_has_next(iter)) {
		char *nodestr = iterator_next(iter);
		nodes[nodecnt++] = hx_node_parse(nodestr);
		if(nodecnt == 3) {
			hx_add_triple(*store, nodes[0], nodes[1], nodes[2]);
			hx_free_node(nodes[0]);
			hx_free_node(nodes[1]);
			hx_free_node(nodes[2]);
			nodecnt = 0;
		}
		free(nodestr);
	}
	
	MPI_RDFIO_DEBUG("%i: Destroying iterator and closing file.\n", rank);
	
	iterator_destroy(iter);
	
	MPI_File_close(&file);

	MPI_Info_free(&info);

	return 1;
}

hx_node* _mpi_rdfio_to_hx_node_p(char* ntnode) {
	switch(ntnode[0]) {
		case '<': {
			ntnode[strlen(ntnode)-1] = '\0';
			return hx_new_node_resource(&ntnode[1]);
		}
		case '_': {
			return hx_new_node_blank(&ntnode[2]);
		}
		case '"': {
			int max_idx = strlen(ntnode) - 1;
			switch(ntnode[max_idx]) {
				case '"': {
					ntnode[max_idx] = '\0';
					return hx_new_node_literal(&ntnode[1]);
				}
				case '>': {
					ntnode[max_idx] = '\0';
					char* last_quote_p = strrchr(ntnode, '"');
					if(last_quote_p == NULL) {
						fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; expected typed literal, but found %s\n", __FILE__, __LINE__, ntnode);
						return NULL;
					}
					last_quote_p[0] = '\0';
					return (hx_node*)hx_new_node_dt_literal(&ntnode[1], &last_quote_p[4]);
				}
				default: {
					char* last_quote_p = strrchr(ntnode, '"');
					if(last_quote_p == NULL) {
						fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; expected literal with language tag, but found %s\n", __FILE__, __LINE__, ntnode);
						return NULL;
					}
					last_quote_p[0] = '\0';
					return (hx_node*)hx_new_node_lang_literal(&ntnode[1], &last_quote_p[2]);
				}
			}
		}
		default: {
			fprintf(stderr, "%s:%u: Error in _mpi_rdfio_to_hx_node_p; invalid N-triples node %s\n", __FILE__, __LINE__, ntnode);
			return NULL;
		}
	}
}


int _mpi_rdfio_compare_string(const void* s1, const void* s2, void* p) {
	return strcmp((char*)s1, (char*)s2);
}

int _mpi_rdfio_compare_hx_node_id(const void* id1, const void* id2, void* p) {
	hx_node_id *hid1 = (hx_node_id*) id1;
	hx_node_id *hid2 = (hx_node_id*) id2;
	return id1 < id2 ? -1 : (id1 > id2 ? 1 : 0);
}

void _mpi_rdfio_destroy_entry_with_free(void* k, void* v, void* p) {
	if(k != NULL) {
		free(k);
	}
	if(v != NULL) {
		free(v);
	}
}

int _mpi_rdfio_send_lookup(async_mpi_session* ses, void* p) {
	void **pack = (void**)p;
	int *commsize = (int*)pack[0];
	iterator_t iter = (iterator_t)pack[1];
	if(!iterator_has_next(iter)) {
		return 0;
	}
	map_entry_t entry = (map_entry_t) iterator_next(iter);
	char *node = (char*)entry->key;
	int *lid = (int*)entry->value;
	size_t bufsize = sizeof(int) + strlen(node) + 1;
	unsigned char *buf = malloc(bufsize);
	if(buf == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate buffer of %u bytes for sending %lu %s\n", __FILE__, __LINE__, bufsize, *lid, node);
		return -1;
	}
	memcpy(buf, lid, sizeof(int));
	strcpy((char*)&buf[sizeof(int)], node);

	int hash = hx_util_hash_string(node) % *commsize;

	MPI_RDFIO_DEBUG("\tSending %i %s to %i.\n", *lid, node, hash);

	// preemptively freeing item to save memory
// 	free(lid);
// 	entry->value = NULL;

	async_mpi_session_reset3(ses, buf, bufsize, hash, ses->flags | ASYNC_MPI_FLAG_FREE_BUF);
	return 1;
}

int _mpi_rdfio_recv_lookup(async_mpi_session* ses, void* p) {
	void** pack = (void**)p;
	size_t *lookupsize = pack[0];
	size_t *lookupmaxsize = pack[1];
	_mpi_rdfio_lookup_record **records = pack[2];
	hx_nodemap *gid2node = pack[3];

	if(*lookupsize >= *lookupmaxsize) {
		MPI_RDFIO_DEBUG("\tResizing lookup table.\n", NULL);
		*lookupmaxsize *= 2;
		_mpi_rdfio_lookup_record *tmp = realloc(*records, (*lookupmaxsize)*sizeof(_mpi_rdfio_lookup_record));
		if(tmp == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for resizing lookup record array.\n", __FILE__, __LINE__, (*lookupmaxsize)*sizeof(_mpi_rdfio_lookup_record));
			return -1;
		}
		*records = tmp;
	}
	_mpi_rdfio_lookup_record *lookup = &(*records)[(*lookupsize)++];
	lookup->pid = ses->peer;
	memcpy(&(lookup->lid), ses->buf, sizeof(int));
	hx_node *node = _mpi_rdfio_to_hx_node_p(&(((char*)ses->buf)[sizeof(int)]));
	
	
	lookup->gid = hx_nodemap_add_node_mpi(gid2node, node);
	
	hx_free_node( node );
	
	MPI_RDFIO_DEBUG("\tReceived lookup[pid=%i, lid=%i, gid=%"PRIdHXID"].\n", lookup->pid, lookup->lid, lookup->gid);
	MPI_RDFIO_DEBUG("\tReceived lookup[pid=%i, lid=%i, gid=%"PRIdHXID"].\n", (*records)[(*lookupsize)-1].pid, (*records)[(*lookupsize)-1].lid, (*records)[(*lookupsize)-1].gid);
	
	return 1;
}

int _mpi_rdfio_send_answer(async_mpi_session* ses, void* p) {
	void** pack = (void**)p;
	size_t *count = pack[0];
	size_t *size = pack[1];
	_mpi_rdfio_lookup_record **records = pack[2];

	MPI_RDFIO_DEBUG("\tsend_answer count=%u size=%u records=%p *records=%p\n", *count, *size, records, *records);

	if(*count >= *size) {
		return 0;
	}
	_mpi_rdfio_lookup_record *lookup = &((*records)[(*count)++]); 

	MPI_RDFIO_DEBUG("\tSending answer pid:%i & lid:%i ==> gid:%lu\n", lookup->pid, lookup->lid, lookup->gid);

	memcpy(ses->buf, lookup, sizeof(hx_node_id) + sizeof(int));
	
	async_mpi_session_reset3(ses, ses->buf, ses->count, lookup->pid, ses->flags | ASYNC_MPI_FLAG_FREE_BUF);
	return 1;
}

int _mpi_rdfio_recv_answer(async_mpi_session* ses, void* p) {
	void** pack = (void**)p;
	size_t *max = (size_t*)pack[0];
	size_t *count = (size_t*)pack[1];
	hx_node_id **lid2gid = (hx_node_id**)pack[2];

	MPI_RDFIO_DEBUG("\trecv_answer max=%u count=%u lid2gid=%p\n", *max, *count, lid2gid);

	unsigned char *buf = (unsigned char*)ses->buf;
	int lid;
	memcpy(&lid, &buf[sizeof(hx_node_id)], sizeof(int));
	memcpy(&((*lid2gid)[lid]), buf, sizeof(hx_node_id));

	MPI_RDFIO_DEBUG("\tReceived answer lid:%i ==> gid:%lu\n", lid, (*lid2gid)[lid]);

	return 1;
}
