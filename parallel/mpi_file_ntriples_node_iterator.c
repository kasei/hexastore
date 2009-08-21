#include "parallel/mpi_file_ntriples_node_iterator.h"
#include "genmap/iterator.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpi.h"

#define _MPI_FILE_NTRIPLES_NODE_ITERATOR_SUBJECT 0
#define _MPI_FILE_NTRIPLES_NODE_ITERATOR_PREDICATE 1
#define _MPI_FILE_NTRIPLES_NODE_ITERATOR_OBJECT 2

typedef struct {
	iterator_t iter;
	char *ntriple;
	char *mark;
	unsigned char part;
} _mpi_file_ntriples_node_iterator_state_type;

typedef _mpi_file_ntriples_node_iterator_state_type* _mpi_file_ntriples_node_iterator_state_t;

int _mpi_file_ntriples_node_iterator_has_next(void*);
void* _mpi_file_ntriples_node_iterator_next(void*);
void _mpi_file_ntriples_node_iterator_destroy(void*);

iterator_t mpi_file_ntriples_node_iterator_create(MPI_File file, MPI_Offset start, MPI_Offset amount, size_t bufsize, MPI_Comm comm) {
	_mpi_file_ntriples_node_iterator_state_t state = malloc(sizeof(_mpi_file_ntriples_node_iterator_state_type));
	if(state == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_ntriples_node_iterator_create; unable to allocate %u bytes for internal state.\n", __FILE__, __LINE__, sizeof(_mpi_file_ntriples_node_iterator_state_type));
		return NULL;
	}
	state->ntriple = NULL;
	state->mark = NULL;
	state->part = _MPI_FILE_NTRIPLES_NODE_ITERATOR_SUBJECT;
	state->iter = mpi_file_ntriples_iterator_create(file, start, amount, bufsize, comm);
	if(state->iter == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_ntriples_node_iterator_create; unable to allocate underlying iterator.\n", __FILE__, __LINE__);
		free(state);
		return NULL;
	}
	iterator_t iter = iterator_create(&_mpi_file_ntriples_node_iterator_has_next, &_mpi_file_ntriples_node_iterator_next, &_mpi_file_ntriples_node_iterator_destroy, state);
	if(iter == NULL) {
		fprintf(stderr, "%s:%u: Error in mpi_file_ntriples_node_iterator_create; unable to allocate iterator.\n", __FILE__, __LINE__);
		iterator_destroy(state->iter);
		free(state);
	}
	return iter;
}

int _mpi_file_ntriples_node_iterator_has_next(void* s) {
	_mpi_file_ntriples_node_iterator_state_t state = (_mpi_file_ntriples_node_iterator_state_t) s;
	return state->ntriple != NULL || iterator_has_next(state->iter);
}

void* _mpi_file_ntriples_node_iterator_next(void* s) {
	_mpi_file_ntriples_node_iterator_state_t state = (_mpi_file_ntriples_node_iterator_state_t) s;
	if(state->ntriple == NULL) {
		state->ntriple = (char*)iterator_next(state->iter);
		state->mark = state->ntriple;
		state->part = _MPI_FILE_NTRIPLES_NODE_ITERATOR_SUBJECT;
	}
	switch(state->part) {
		case _MPI_FILE_NTRIPLES_NODE_ITERATOR_SUBJECT:
		case _MPI_FILE_NTRIPLES_NODE_ITERATOR_PREDICATE: {
			char *start = state->mark;
			state->mark = strpbrk(state->mark, " \t");
			if(state->mark == NULL) {
				fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_node_iterator_next; malformed ntriple %s", __FILE__, __LINE__, state->ntriple);
				return NULL;
			}
			state->mark[0] = '\0';
			size_t len = strlen(start);
			char *node = malloc(len + 1);
			if(node == NULL) {
				fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_node_iterator_next; unable to allocate %u bytes for node.\n", __FILE__, __LINE__, len + 1);
				return NULL;
			}
			strcpy(node, start);
			state->mark[0] = ' ';
			while(state->mark[0] == ' ' || state->mark[0] == '\t') {
				state->mark = &state->mark[1];
			}
			state->part++;
			return node;
		}
		case _MPI_FILE_NTRIPLES_NODE_ITERATOR_OBJECT: {
			char *start = state->mark;
			state->mark = strrchr(state->mark, '.');
			if(state->mark == NULL) {
				fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_node_iterator_next; malformed ntriple %s", __FILE__, __LINE__, state->ntriple);
				return NULL;
			}
			state->mark[0] = '\0';
			size_t len = strlen(start);
			while(start[len-1] == ' ' || start[len-1] == '\t') {
				start[len-1] = '\0';
				len--;
			}
			char *node = malloc(len + 1);
			if(node == NULL) {
				fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_node_iterator_next; unable to allocate %u bytes for node.\n", __FILE__, __LINE__, len + 1);
				return NULL;
			}
			strcpy(node, start);
			free(state->ntriple);
			state->ntriple = NULL;
			state->part++; // just to cause sanity check in case of problem
			return node;
		}
		default: {
			fprintf(stderr, "%s:%u: Error in _mpi_file_ntriples_node_iterator_next; unhandled case %u should never happen.\n", __FILE__, __LINE__, (unsigned int)state->part);
			return NULL;
		}
	}
}

void _mpi_file_ntriples_node_iterator_destroy(void* s) {
	_mpi_file_ntriples_node_iterator_state_t state = (_mpi_file_ntriples_node_iterator_state_t) s;
	iterator_destroy(state->iter);
	free(state);
}
