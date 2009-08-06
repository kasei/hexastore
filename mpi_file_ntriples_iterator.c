#include "mpi_file_ntriples_iterator.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	MPI_Comm comm;
	iterator_t iter;
	unsigned char *first_frag;
	unsigned char *last_frag;
	size_t index;
	buffer_type *buffer;
} _mpi_file_ntriples_iterator_state_type;

typedef _mpi_file_ntriples_iterator_state_type* _mpi_file_ntriples_iterator_state_t;

int _mpi_file_ntriples_iterator_has_next(void*);
void* _mpi_file_ntriples_iterator_next(void*);
void _mpi_file_ntriples_iterator_destroy(void*);

iterator_t mpi_file_ntriples_iterator_create(MPI_File file, MPI_Offset start, MPI_Offset amount, size_t bufsize, MPI_Comm comm) {
	_mpi_file_ntriples_iterator_state_t state = (_mpi_file_ntriples_iterator_state_t)malloc(sizeof(_mpi_file_ntriples_iterator_state_type));
	if(state == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_ntriples_iterator_create; unable to allocate %u bytes.\n", __FILE__, __LINE__, sizeof(_mpi_file_ntriples_iterator_state_type));
		return NULL;
	}
	state->iter = mpi_file_iterator_create(file, start, amount, bufsize);
	if(state->iter == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_ntriples_iterator_create; unable to allocate mpi_file_iterator.\n", __FILE__, __LINE__);
		free(state);
		return NULL;
	}
	state->first_frag = NULL;
	state->last_frag = NULL;
	state->index = 0;
	state->buffer = NULL;
	state->comm = comm;
	iterator_t iter = iterator_create(&_mpi_file_ntriples_iterator_has_next, &_mpi_file_ntriples_iterator_next, &_mpi_file_ntriples_iterator_destroy, state);
	if(iter == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_ntriples_iterator_create; unable to allocate underlying iterator.\n", __FILE__, __LINE__);
		free(state->iter);
		free(state);
	}
	return iter;
}

int _mpi_file_ntriples_iterator_has_next(void* s) {
	_mpi_file_ntriples_iterator_state_t state = (_mpi_file_ntriples_iterator_state_t) s;
	return (state->buffer != NULL && state->index < state->buffer->size) || state->first_frag != NULL || iterator_has_next(state->iter);
}

void* _mpi_file_ntriples_iterator_next(void* s) {
	_mpi_file_ntriples_iterator_state_t state = (_mpi_file_ntriples_iterator_state_t) s;
	unsigned char *mark, *nt;
	size_t len;
	if(state->buffer == NULL || state->index >= state->buffer->size) {
		if(iterator_has_next(state->iter)) {
			state->buffer = iterator_next(state->iter);
			state->index = 0;
		} else {
			nt = state->first_frag;
			state->first_frag = NULL;
			return nt;
		}
	}
	char lastchar = state->buffer->bytes[state->buffer->size - 1];
	state->buffer->bytes[state->buffer->size - 1] = '\0';
	mark = (unsigned char*)strchr((char*)&(state->buffer->bytes[state->index]), '\n');
	state->buffer->bytes[state->buffer->size - 1] = lastchar;
	if(mark == NULL && lastchar == '\n') {
		mark = &(state->buffer->bytes[state->buffer->size - 1]);
	}
	if(mark == NULL) {
		len = state->buffer->size - state->index;
	} else {
		mark[0] = '\0';
		len = strlen((char*)&(state->buffer->bytes[state->index])) + 1;
		mark[0] = '\n';
	}
	nt = malloc(len + 1);
	if(nt == NULL) {
		fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_iterator_next; unable to allocate %u bytes for ntriple.\n", __FILE__, __LINE__, len + 1);
		return NULL;
	}
	memcpy(nt, &(state->buffer->bytes[state->index]), len);
	nt[len] = '\0';
	state->index += len;
	if(mark == NULL) {
		if(state->last_frag == NULL) { 
			state->last_frag = nt;
		} else {
			size_t last_frag_len = strlen((char*)state->last_frag);
			state->last_frag = realloc(state->last_frag, last_frag_len + len + 1);
			if(state->last_frag == NULL) {
				fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_iterator_next; unable to allocate %u bytes for extending last fragment.\n", __FILE__, __LINE__, last_frag_len + len + 1);
				return NULL;
			}
			memcpy((char*)&(state->last_frag[last_frag_len]), (char*)nt, len + 1);
			free(nt);
		}
		nt = (unsigned char*) _mpi_file_ntriples_iterator_next(s);
		len = strlen((char*)nt);

		size_t last_frag_len = strlen((char*)state->last_frag);
		state->last_frag = realloc(state->last_frag, last_frag_len + len + 1);
		if(state->last_frag == NULL) {
			fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_iterator_next; unable to allocat %u bytes for completing last fragment.\n", __FILE__, __LINE__, last_frag_len + len + 1);
			return NULL;
		}
		memcpy((char*)&(state->last_frag[last_frag_len]), (char*)nt, len + 1);
		free(nt);
		nt = state->last_frag;
		state->last_frag = NULL;
		return nt;
	}
	if(state->first_frag == NULL) {
		int rank, commsize;
		MPI_Comm_rank(state->comm, &rank);
		MPI_Comm_size(state->comm, &commsize);
		MPI_Request sreq, rreq;
		MPI_Status status;
		int count = 0;
		unsigned char *tmp = NULL;
		if(rank > 0) {
			MPI_Isend(nt, len, MPI_BYTE, rank - 1, 0, state->comm, &sreq);
		} else {
			MPI_Isend(nt, len, MPI_BYTE, commsize - 1, 0, state->comm, &sreq);
		}
		do {
			MPI_Iprobe(MPI_ANY_SOURCE, 0, state->comm, &count, &status);
		} while(!count);
		MPI_Get_count(&status, MPI_BYTE, &count);
		tmp = malloc(count + 1);
		if(tmp == NULL) {
			fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_iterator_next; unable to allocate %u bytes for receiving first fragment.\n", __FILE__, __LINE__, count + 1);
			return NULL;
		}
		tmp[count] = '\0';
		MPI_Irecv(tmp, count, MPI_BYTE, status.MPI_SOURCE, 0, state->comm, &rreq);
		int test;
		do {
			MPI_Test(&sreq, &test, &status);
		} while(!test);
		free(nt);
		do {
			MPI_Test(&rreq, &test, &status);
		} while(!test);
		state->first_frag = tmp;
		nt = (unsigned char*) _mpi_file_ntriples_iterator_next(s);
		len = strlen((char*)nt);
	}
	return nt;
}

void _mpi_file_ntriples_iterator_destroy(void* s) {
	_mpi_file_ntriples_iterator_state_t state = (_mpi_file_ntriples_iterator_state_t) s;
	iterator_destroy(state->iter);
	free(state->first_frag);
	free(state);
}
