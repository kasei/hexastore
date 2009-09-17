#include "mentok/parallel/mpi_file_iterator.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
	MPI_File file;
	MPI_Offset start;
	MPI_Offset offset;
	MPI_Offset amount;
	buffer_type buffer;
} _mpi_file_iterator_state_type;

typedef _mpi_file_iterator_state_type* _mpi_file_iterator_state_t;

int _mpi_file_iterator_has_next(void*);
void* _mpi_file_iterator_next(void*);
void _mpi_file_iterator_destroy(void*);

iterator_t mpi_file_iterator_create(MPI_File file, MPI_Offset start, MPI_Offset amount, size_t bufsize) {
	_mpi_file_iterator_state_t state = malloc(sizeof(_mpi_file_iterator_state_type));
	if(state == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_iterator_create; unable to allocate %u bytes.\n", __FILE__, __LINE__, sizeof(_mpi_file_iterator_state_type));
		return NULL;
	}
	state->file = file;
	state->start = start;
	state->offset = 0;
	state->amount = amount;
	state->buffer.size = bufsize;
	state->buffer.bytes = malloc(bufsize);
	if(state->buffer.bytes == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_iterator_create; unable to allocate %u bytes for buffer.\n", __FILE__, __LINE__, bufsize);
		free(state);
		return NULL;
	}
	iterator_t iter = iterator_create(&_mpi_file_iterator_has_next, &_mpi_file_iterator_next, &_mpi_file_iterator_destroy, state);
	if(iter == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_iterator_create; unable to allocate underlying iterator.\n", __FILE__, __LINE__);
		free(state->buffer.bytes);
		free(state);
	}
	return iter;
}

int _mpi_file_iterator_has_next(void *s) {
	_mpi_file_iterator_state_t state = (_mpi_file_iterator_state_t) s;
	return state->offset < state->amount;
}

void* _mpi_file_iterator_next(void *s) {
	_mpi_file_iterator_state_t state = (_mpi_file_iterator_state_t) s;
	if(state->amount - state->offset < state->buffer.size) {
		state->buffer.size = state->amount - state->offset;
	}
	MPI_Status status;
	MPI_File_read_at(state->file, state->start + state->offset, state->buffer.bytes, state->buffer.size, MPI_BYTE, &status);
	state->offset += state->buffer.size;
	return &(state->buffer);
}

void _mpi_file_iterator_destroy(void *s) {
	_mpi_file_iterator_state_t state = (_mpi_file_iterator_state_t) s;
	free(state->buffer.bytes);
	free(state);
}
