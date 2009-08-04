/**
 * Jesse Weaver
 * Tetherless World Constellation
 * Rensselaer Polytechnic Institute
 */

#ifndef _ASYNC_MPI_H
#define _ASYNC_MPI_H

#include "async.h"
#include <mpi.h>

#define ASYNC_MPI_FLAG_IS_SENDING 1
#define ASYNC_MPI_FLAG_SHOULD_SEND_COUNT 2
#define ASYNC_MPI_FLAG_FREE_BUF 4
#define ASYNC_MPI_FLAG_FREE_REQUEST 8

#define ASYNC_MPI_STATE_SEND_OR_RECV 0
#define ASYNC_MPI_STATE_TEST 1
#define ASYNC_MPI_STATE_SEND_OR_RECV2 2
#define ASYNC_MPI_STATE_TEST2 3
#define ASYNC_MPI_STATE_SUCCESS 4
#define ASYNC_MPI_STATE_FAILURE 5

typedef struct {
	void* buf;
	int* tmp;
	int count;
	int tag;
	MPI_Datatype datatype;
	int peer;
	MPI_Comm comm;
	MPI_Request* request;
	int state;
	unsigned char flags;
} async_mpi_session;

int async_mpi_session_reset(async_mpi_session* ses, void* buf, int count, MPI_Datatype datatype, int peer, int tag, MPI_Comm comm, MPI_Request* request, unsigned char flags);

int async_mpi_session_reset2(async_mpi_session* ses, void* buf, int count, MPI_Datatype datatype, int peer, unsigned char flags); 

int async_mpi_session_reset3(async_mpi_session* ses, void* buf, int count, int peer, unsigned char flags);

async_mpi_session* async_mpi_session_create(void* buf, int count, MPI_Datatype datatype, int peer, int tag, MPI_Comm comm, MPI_Request* request, unsigned char flags);

async_mpi_session* async_mpi_session_create2(void* buf, int count, MPI_Datatype datatype, int peer, unsigned char flags); 

async_mpi_session* async_mpi_session_create3(void* buf, int count, int peer, unsigned char flags);

void async_mpi_session_destroy(async_mpi_session* session);

enum async_status async_mpi(void* session);

#endif /* _ASYNC_MPI_H */
