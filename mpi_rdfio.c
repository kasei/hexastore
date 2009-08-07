#include "mpi_rdfio.h"
#include "mpi_file_ntriples_node_iterator.h"
#include "async_des.h"
#include "genmap/avl_tree_map.h"
#include "parallel.h"

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

// #define MPI_RDFIO_DEBUG(s, ...) fprintf(stderr, "%s:%u: "s"", __FILE__, __LINE__, __VA_ARGS__)
#define MPI_RDFIO_DEBUG(s, ...)

int mpi_rdfio_readnt(char *filename, char *mapfilename, size_t bufsize, hx_hexastore **store, hx_storage_manager **manager, MPI_Comm comm) {
	MPI_File file;
	MPI_Offset filesize;
	MPI_Info info;
	int rank, commsize;

	MPI_Comm_rank(comm, &rank);
	MPI_Comm_size(comm, &commsize);

	MPI_RDFIO_DEBUG("%i: Creating list of triple ids.\n", rank);

	size_t listsize = 0;
	size_t listmaxsize = 3*256;
	hx_node_id* list = malloc(listmaxsize*sizeof(hx_node_id));
	if(list == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for triples.\n", __FILE__, __LINE__, listmaxsize);
		return -1;
	}

	MPI_RDFIO_DEBUG("%i: Creating node2lid map (map_t).\n", rank);

	map_t node2lid;
	node2lid = avl_tree_map_create(&_mpi_rdfio_compare_string, NULL, &_mpi_rdfio_destroy_entry_with_free, NULL, NULL);
	if(node2lid == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate node2lid map.\n", __FILE__, __LINE__);
		free(list);
		return -1;
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

	int lids = 0;

	MPI_RDFIO_DEBUG("%i: Reading triples from %llu bytes at offset %llu of file with length %llu bytes.\n", rank, my_chunk, (MPI_Offset)(rank*chunk), filesize);

	iterator_t iter = mpi_file_ntriples_node_iterator_create(file, rank*chunk, my_chunk, bufsize, comm);
	
	while(iterator_has_next(iter)) {
		if(listsize >= listmaxsize) {
			listmaxsize *= 2;
			hx_node_id *tmp = realloc(list, listmaxsize*sizeof(hx_node_id));
			if(tmp == NULL) {
				fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for extending triples list.\n", __FILE__, __LINE__, listmaxsize);
				iterator_destroy(iter);
				MPI_Info_free(&info);
				map_destroy(node2lid);
				free(list);
				return -1;	
			}
			list = tmp;
		}
		char *node = iterator_next(iter);
		MPI_RDFIO_DEBUG("%i:\tnode = %s\n", rank, node);
		int *lid = map_get(node2lid, node);
		MPI_RDFIO_DEBUG("%i:\tmap_get(%s) == %p\n", rank, node, lid);
		if(lid == NULL) {
			lid = malloc(sizeof(int));
			if(lid == NULL) {
				fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for lid.\n", __FILE__, __LINE__, sizeof(int));
				iterator_destroy(iter);
				MPI_Info_free(&info);
				map_destroy(node2lid);
				free(list);
				return -1;
			}
			*lid = lids++;
			MPI_RDFIO_DEBUG("%i:\tmap_put(%s, %i)\n", rank, node, *lid);
			map_put(node2lid, node, lid);
			MPI_RDFIO_DEBUG("%i:\tmap_put(%s, %i) CHECK!\n", rank, node, *lid);
		} else {
			free(node);
		}
		MPI_RDFIO_DEBUG("%i:\tlist[%u] = %i\n", rank, listsize, *lid);
		list[listsize++] = (hx_node_id) *lid;
	}

	MPI_RDFIO_DEBUG("%i: Resizing triples list/table.\n", rank);

	list = realloc(list, listsize*sizeof(hx_node_id));
	listmaxsize = listsize;

	iterator_destroy(iter);	

	MPI_File_close(&file);

	MPI_RDFIO_DEBUG("%i: Creating gid2node map (hx_nodemap*).\n", rank);

	hx_nodemap *gid2node = hx_new_nodemap();
	if(gid2node == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate gid2node map.\n", __FILE__, __LINE__);
		map_destroy(node2lid);
		free(list);
		return -1;
	}

	MPI_RDFIO_DEBUG("%i: Creating lookups table (_mpi_rdfio_lookup_record*).\n", rank);

	size_t lookupsize = 0;
	size_t lookupmaxsize = 256;
	/*
	_mpi_rdfio_lookup_record** lookups = malloc(sizeof(_mpi_rdfio_lookup_record*));
	if(lookups != NULL) {
		*lookups = malloc(lookupmaxsize*sizeof(_mpi_rdfio_lookup_record));
	}
	*/
	//if(lookups == NULL || *lookups == NULL) {
	_mpi_rdfio_lookup_record *lookups = malloc(lookupmaxsize*sizeof(_mpi_rdfio_lookup_record));
	if(lookups == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for lookup records.\n", __FILE__, __LINE__, sizeof(_mpi_rdfio_lookup_record*) + lookupmaxsize*sizeof(_mpi_rdfio_lookup_record));
		hx_free_nodemap(gid2node);
		map_destroy(node2lid);
		free(list);
		return -1;
	}

	MPI_RDFIO_DEBUG("%i: Getting iterator of node2lid.\n", rank);
	
	iter = map_entry_iterator(node2lid);
	if(iter == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate iterator on node2lid map.\n", __FILE__, __LINE__);
		free(lookups);
		hx_free_nodemap(gid2node);
		map_destroy(node2lid);
		free(list);
		return -1;
	}


	void* pack[4];
	pack[0] = &lookupsize;
	pack[1] = &lookupmaxsize;
	pack[2] = &lookups;
	pack[3] = gid2node;

	void* pack2[3];
	pack2[0] = &commsize;
	pack2[1] = iter;

	MPI_RDFIO_DEBUG("%i: Creating lookup session.\n", rank);

	async_des_session *des = async_des_session_create(2, &_mpi_rdfio_send_lookup, &pack2, 4, &_mpi_rdfio_recv_lookup, &pack, 0);
	if(des == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate des session.\n", __FILE__, __LINE__);
		iterator_destroy(iter);
		hx_free_nodemap(gid2node);
		map_destroy(node2lid);
		free(list);
		return -1;
	}

	MPI_RDFIO_DEBUG("%i: Sending lookups.\n", rank);

	enum async_status astat;
	do {
		astat = async_des(des);
	} while(astat == ASYNC_PENDING);

	if(astat == ASYNC_FAILURE) {
		fprintf(stderr, "%s:%u: Error; des session failed.\n", __FILE__, __LINE__);
		async_des_session_destroy(des);
		free(lookups);
		iterator_destroy(iter);
		hx_free_nodemap(gid2node);
		map_destroy(node2lid);
		free(list);
		return -1;
	}

	async_des_session_destroy(des);

	iterator_destroy(iter);

	map_destroy(node2lid);

	MPI_RDFIO_DEBUG("%i: Resizing lookups table from %u to %u.\n", rank, lookupmaxsize, lookupsize);

	lookups = realloc(lookups, lookupsize * sizeof(_mpi_rdfio_lookup_record));
	lookupmaxsize = lookupsize;
	lookupsize = 0;
	pack[2] = &lookups;

	MPI_RDFIO_DEBUG("%i: Writing gid2node map to %s.\n", rank, mapfilename);

	MPI_File_open(MPI_COMM_SELF, mapfilename, MPI_MODE_WRONLY | MPI_MODE_CREATE, info, &file);

	hx_nodemap_write_mpi(gid2node, file);
	hx_free_nodemap(gid2node);

	MPI_File_close(&file);

	MPI_Info_free(&info);

	MPI_RDFIO_DEBUG("%i: Creating lid2gid map (hx_node_id*).\n", rank);

	hx_node_id *lid2gid = malloc(lids*sizeof(hx_node_id));
	if(lid2gid == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for lid2gid.\n", __FILE__, __LINE__, lids*sizeof(hx_node_id));
		free(list);
		return -1;
	}

	size_t lidcount = 0;

	pack2[0] = &lids;
	pack2[1] = &lidcount;
	pack2[2] = &lid2gid;

	MPI_RDFIO_DEBUG("%i: Creating answer session.\n", rank);

	des = async_des_session_create(2, &_mpi_rdfio_send_answer, &pack, 4, &_mpi_rdfio_recv_answer, &pack2, sizeof(hx_node_id) + sizeof(int));
	if(des == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate second des session.\n", __FILE__, __LINE__);
		free(lid2gid);
		free(list);
		return -1;
	}

	MPI_RDFIO_DEBUG("%i: Sending answers.\n", rank);

	do {
		astat = async_des(des);
	} while(astat == ASYNC_PENDING);

	if(astat == ASYNC_FAILURE) {
		fprintf(stderr, "%s:%u: Error; second des session failed.\n", __FILE__, __LINE__);
		async_des_session_destroy(des);
		free(lid2gid);
		free(list);
		return -1;
	}

	async_des_session_destroy(des);
	free(lookups);
	
	MPI_RDFIO_DEBUG("%i: Translating lids to gids.\n", rank);
	
	size_t i;
	for(i = 0; i < listsize; i++) {
		MPI_RDFIO_DEBUG("%i: %u <= %u\n", rank, (unsigned int)lid2gid[list[i]], (unsigned int)list[i]);
		list[i] = lid2gid[list[i]];
	}

	// don't need to keep lid2gid, right?
	free(lid2gid);

	int created_manager = 0;

	MPI_RDFIO_DEBUG("%i: %p == %p ? Creating manager.\n", rank, *manager, NULL);

	if(*manager == NULL) {
		*manager = hx_new_memory_storage_manager();
		if(*manager == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot allocate storage manager.\n", __FILE__, __LINE__);
			free(list);
			return -1;
		}
		created_manager = 1;
	}

	MPI_RDFIO_DEBUG("%i: %p == %p ? Creating store.\n", rank, *store, NULL);

	if(*store == NULL) {
		*store = hx_new_hexastore(*manager);
		if(*store == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot allocate hexastore.\n", __FILE__, __LINE__);
			if(created_manager) {
				hx_free_storage_manager(*manager);
			}
			free(list);
			return -1;
		}
	}

	MPI_RDFIO_DEBUG("%i: Changing triples to use gids.\n", rank);

	int j = 2;
	size_t r = 0;
	size_t realloc_at = 256;
	hx_node_id trip[3];
	int done = 0;
	for(i = listsize - 1; !done; i--) {
		trip[j--] = list[i];
		if(j < 0) {
			MPI_RDFIO_DEBUG("%i:\tadd triple %"PRIuHXID" %"PRIuHXID" %"PRIuHXID"\n", rank, trip[0], trip[1], trip[2]);
			hx_add_triple_id(*store, *manager, trip[0], trip[1], trip[2]);
			j = 2;
		}
		if(++r >= realloc_at) {
			r = 0;
			list = realloc(list, (i+1)*sizeof(hx_node_id));
			if(list == NULL) {
				fprintf(stderr, "%s:%u: realloced to smaller array (%u < %u  ? %u); what's the problem?\n", __FILE__, __LINE__, (i+1), listsize, sizeof(hx_node_id));
			}
		}
		done = i == 0;
	}

	if(list != NULL) {
		free(list);
	}

	MPI_RDFIO_DEBUG("%i: Returning successfully.\n", rank);

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

	int hash = hx_triple_hash_string(node) % *commsize;

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
	
	MPI_RDFIO_DEBUG("\tReceived lookup[pid=%i, lid=%i, gid=%"PRIuHXID"].\n", lookup->pid, lookup->lid, lookup->gid);
	MPI_RDFIO_DEBUG("\tReceived lookup[pid=%i, lid=%i, gid=%"PRIuHXID"].\n", (*records)[(*lookupsize)-1].pid, (*records)[(*lookupsize)-1].lid, (*records)[(*lookupsize)-1].gid);
	
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

int mpi_rdfio_readids(char *filename, size_t bufsize, hx_hexastore **store, hx_storage_manager **manager, MPI_Comm comm) {
	MPI_File file;
	MPI_Info info;
	MPI_Offset size, triplesize;
	int commrank, commsize;

	triplesize = 3 * sizeof(hx_node_id);

	if(bufsize % triplesize != 0) {
		fprintf(stderr, "%s:%u: Warning; buffer size %u is not a multiple of triple size %u.  Truncating buffer size to %u bytes.\n", __FILE__, __LINE__, bufsize, (size_t)triplesize, (size_t)(bufsize - (bufsize % triplesize)));
		bufsize -= bufsize % triplesize;
	}

	unsigned char *buffer = calloc(bufsize, sizeof(unsigned char));
	if(buffer == NULL) {
		fprintf(stderr, "%s:%u: Error; cannot allocate %u bytes for buffer.\n", __FILE__, __LINE__, bufsize);
		return -1;
	}

	MPI_Comm_rank(comm, &commrank);
	MPI_Comm_size(comm, &commsize);

	MPI_Info_create(&info);
	MPI_File_open(comm, filename, MPI_MODE_RDONLY, info, &file);

	MPI_File_get_size(file, &size);

	MPI_Offset chunk = size % triplesize;
	if(chunk != 0) {
		fprintf(stderr, "%s:%u: Warning; file ends with fragmented triple.  Ignoring fragment and continuing.\n", __FILE__, __LINE__);
		size -= chunk;
	}

	MPI_Offset numtriples = size / triplesize;
	chunk = numtriples / commsize;
	MPI_Offset mychunk = chunk;
	if(commrank == commsize - 1) {
		mychunk += numtriples % commsize;
	}

	int created_manager = 0;

	if(*manager == NULL) {
		*manager = hx_new_memory_storage_manager();
		if(*manager == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot allocate storage manager.\n", __FILE__, __LINE__);
			return -1;
		}
		created_manager = 1;
	}

	if(*store == NULL) {
		*store = hx_new_hexastore(*manager);
		if(*store == NULL) {
			fprintf(stderr, "%s:%u: Error; cannot allocate hexastore.\n", __FILE__, __LINE__);
			if(created_manager) {
				hx_free_storage_manager(*manager);
			}
			return -1;
		}
	}


	MPI_Offset sofar;
	MPI_Offset start = commrank * chunk * triplesize;
	MPI_Status status;
	hx_node_id ids[3];
//  	fprintf(stderr, "Loading store.\n");
	for(sofar = 0; sofar < mychunk * triplesize; sofar += bufsize) {
		MPI_Offset actualread = bufsize;
		if(mychunk * triplesize - sofar < bufsize) {
			actualread = mychunk * triplesize - sofar;
		}
// 		fprintf(stderr, "Actually reading %llu.\n", actualread);
// 		fprintf(stderr, "MPI_File_read_at(%p, %llu, %p, %llu, %s, %p)\n", file, start + sofar, buffer, actualread, "MPI_BYTE", &status);
		MPI_File_read_at(file, start + sofar, buffer, actualread, MPI_BYTE, &status);
		size_t i;
		for(i = 0; i < actualread; i += sizeof(hx_node_id)) {
			size_t x = i / sizeof(hx_node_id);
			ids[x%3] = *((hx_node_id*)&(buffer[i]));
			if(x % 3 == 2) {
//				fprintf(stderr, "hx_add_triple_id(%p, %p, %llu, %llu, %llu)\n", *store, *manager, ids[0], ids[1], ids[2]);
				hx_add_triple_id(*store, *manager, ids[0], ids[1], ids[2]);
			}
		}
	}

	free(buffer);
	MPI_File_close(&file);
	MPI_Info_free(&info);
	return 1;
}
