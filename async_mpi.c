/**
 * Jesse Weaver
 * Tetherless World Constellation
 * Rensselaer Polytechnic Institute
 */

#include "async_mpi.h"
#include "mpi_safealloc.h"

int async_mpi_session_reset(async_mpi_session* ses, void* buf, int count, MPI_Datatype datatype, int peer, int tag, MPI_Comm comm, MPI_Request* request, unsigned char flags) {
	if(ses->buf != NULL && ses->buf != buf && (ses->flags & ASYNC_MPI_FLAG_FREE_BUF) != 0) {
		safe_free(ses->buf);
		ses->buf = NULL;
	}
	if(ses->request != NULL) {
		if(ses->state > ASYNC_MPI_STATE_SEND_OR_RECV && ses->state < ASYNC_MPI_STATE_SUCCESS) {
			MPI_Status status;
			int flag = 0;
			//printf("Canceling in reset.\n");
			MPI_Cancel(ses->request);
			while(!flag) {
				MPI_Test(ses->request, &flag, &status);
			}
		}
		if((ses->flags & ASYNC_MPI_FLAG_FREE_REQUEST) != 0) {
			safe_free(ses->request);
		}
	}
	ses->buf = buf;
	ses->count = count;
	ses->datatype = datatype;
	ses->peer = peer;
	ses->tag = tag;
	ses->comm = comm;
	ses->request = request;
	ses->flags = flags;
	ses->state = ASYNC_MPI_STATE_SEND_OR_RECV;
	return 1;
}

int async_mpi_session_reset2(async_mpi_session* ses, void* buf, int count, MPI_Datatype datatype, int peer, unsigned char flags) {
	MPI_Request* request = NULL;
	int r = 0;
	if(safe_malloc((void**)&request, sizeof(MPI_Request)) > 0) {
		r = async_mpi_session_reset(ses, buf, count, datatype, peer, 0, MPI_COMM_WORLD, request, flags | ASYNC_MPI_FLAG_FREE_REQUEST);
	}
	return r;
}

int async_mpi_session_reset3(async_mpi_session* ses, void* buf, int count, int peer, unsigned char flags) {
	return async_mpi_session_reset2(ses, buf, count, MPI_BYTE, peer, flags);
}

async_mpi_session* async_mpi_session_create(void* buf, int count, MPI_Datatype datatype, int peer, int tag, MPI_Comm comm, MPI_Request* request, unsigned char flags) {
	async_mpi_session* ses = NULL;
	if(safe_malloc((void**)&ses, sizeof(async_mpi_session)) > 0) {
		ses->buf = buf;
		ses->count = count;
		ses->datatype = datatype;
		ses->peer = peer;
		ses->tag = tag;
		ses->comm = comm;
		ses->request = request;
		ses->flags = flags;
	}
	return ses;
}

async_mpi_session* async_mpi_session_create2(void* buf, int count, MPI_Datatype datatype, int peer, unsigned char flags) {
	async_mpi_session* ses = NULL;
	MPI_Request* request = NULL;
	if(safe_malloc((void**)&request, sizeof(MPI_Request)) > 0) {
		ses = async_mpi_session_create(buf, count, datatype, peer, 0, MPI_COMM_WORLD, request, flags | ASYNC_MPI_FLAG_FREE_REQUEST);
	}
	return ses;
}

async_mpi_session* async_mpi_session_create3(void* buf, int count, int peer, unsigned char flags) {
	return async_mpi_session_create2(buf, count, MPI_BYTE, peer, flags);
}

void async_mpi_session_destroy(async_mpi_session* ses) {
	if((ses->flags & ASYNC_MPI_FLAG_FREE_BUF) != 0) {
		safe_free(ses->buf);
	}
	if(ses->request != NULL) {
		if(ses->state > ASYNC_MPI_STATE_SEND_OR_RECV && ses->state < ASYNC_MPI_STATE_SUCCESS) {
			MPI_Status status;
			int flag = 0;
			//printf("Canceling in destroy.\n");
			MPI_Cancel(ses->request);
			while(!flag) {
				MPI_Test(ses->request, &flag, &status);
			}
		}
		if((ses->flags & ASYNC_MPI_FLAG_FREE_REQUEST) != 0) {
			safe_free(ses->request);
		}
	}
	safe_free(ses);
}

//#define PRINT_SES(s) printf("buf=%p, count=%i, MPI_BYTE, peer=%i, tag=%i, MPI_COMM_WORLD, request=%p, state=%i, flags=%i\n", s->buf, s->count, s->peer, s->tag, *(s->request), s->state, (int)s->flags)
#define PRINT_SES(s)
enum async_status async_mpi(void* session) {
	async_mpi_session* ses = (async_mpi_session*) session;
	switch(ses->state) {
		case ASYNC_MPI_STATE_SEND_OR_RECV: {
			PRINT_SES(ses);
			if((ses->flags & ASYNC_MPI_FLAG_SHOULD_SEND_COUNT) != 0) {
				int r;
				if((ses->flags & ASYNC_MPI_FLAG_IS_SENDING) != 0) {
					//printf("MPI_Isend(%p[%i], %i, MPI_INT, %i, %i, MPI_COMM_WORLD, %p)\n", &(ses->count), ses->count, 1, ses->peer, 0, ses->request);
					r = MPI_Isend(&(ses->count), 2, MPI_INT, ses->peer, 0, ses->comm, ses->request);
				} else {
					//printf("MPI_Irecv(%p[%i], %i, MPI_INT, %i, %i, MPI_COMM_WORLD, %p)\n", &(ses->count), ses->count, 1, ses->peer, 0, ses->request);
					r = MPI_Irecv(&(ses->count), 2, MPI_INT, ses->peer, 0, ses->comm, ses->request);
				}
				if(r != MPI_SUCCESS) {
					ses->state = ASYNC_MPI_STATE_FAILURE;
					return ASYNC_FAILURE;
				}
			} else {
				ses->state = ASYNC_MPI_STATE_SEND_OR_RECV2;
				return async_mpi(ses);
			}
			ses->state = ASYNC_MPI_STATE_TEST;
			// fall-through
		}
		case ASYNC_MPI_STATE_TEST: {
			PRINT_SES(ses);
			int flag;
			MPI_Status status;
			MPI_Test(ses->request, &flag, &status);
			if(!flag) {
				return ASYNC_PENDING;
			}
			if((ses->flags & ASYNC_MPI_FLAG_IS_SENDING) == 0) {
				//printf("count=%i source=%i tag=%i error=%i\n", ses->count, status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR);
				ses->peer = status.MPI_SOURCE;
				//ses->tag = status.MPI_TAG;
				if(safe_realloc(&(ses->buf), ses->buf, ses->count) <= 0) {
					ses->state = ASYNC_MPI_STATE_FAILURE;
					return ASYNC_FAILURE;
				}
			}
			ses->state = ASYNC_MPI_STATE_SEND_OR_RECV2;
			// fall-through
		}
		case ASYNC_MPI_STATE_SEND_OR_RECV2: {
			PRINT_SES(ses);
			int r;
			if((ses->flags & ASYNC_MPI_FLAG_IS_SENDING) != 0) {
				//printf("MPI_Isend(%p[%i,%i], %i, MPI_BYTE, %i, %i, MPI_COMM_WORLD, %p)\n", ses->buf, ((int*)ses->buf)[0], ((int*)ses->buf)[1], ses->count, ses->peer, ses->tag, ses->request);
				r = MPI_Isend(ses->buf, ses->count, ses->datatype, ses->peer, ses->tag, ses->comm, ses->request);
			} else {
				//printf("MPI_Irecv(%p[%i,%i], %i, MPI_BYTE, %i, %i, MPI_COMM_WORLD, %p)\n", ses->buf, ((int*)ses->buf)[0], ((int*)ses->buf)[1], ses->count, ses->peer, ses->tag, ses->request);
				r = MPI_Irecv(ses->buf, ses->count, ses->datatype, ses->peer, ses->tag, ses->comm, ses->request);
			}
			if(r != MPI_SUCCESS) {
				//printf("FAILURE! (from async_mpi)\n");
				ses->state = ASYNC_MPI_STATE_FAILURE;
				return ASYNC_FAILURE;
			}
			ses->state = ASYNC_MPI_STATE_TEST2;
			// fall-through
		}
		case ASYNC_MPI_STATE_TEST2: {
			PRINT_SES(ses);
			int flag = 1;
			MPI_Status status;
			MPI_Test(ses->request, &flag, &status);
			if(!flag) {
				return ASYNC_PENDING;
			}
			//printf("MPI_Test(%p[%i,%i], %i, MPI_BYTE, %i, %i, MPI_COMM_WORLD, %p)\n", ses->buf, ((int*)ses->buf)[0], ((int*)ses->buf)[1], ses->count, ses->peer, ses->tag, ses->request);
			//printf("flag = %i\tSOURCE = %i\tTAG = %i\tERROR = %i\n", flag, status.MPI_SOURCE, status.MPI_TAG, status.MPI_ERROR);
			ses->state = ASYNC_MPI_STATE_SUCCESS;
			// fall-through
		}
		case ASYNC_MPI_STATE_SUCCESS: {
			PRINT_SES(ses);
			return ASYNC_SUCCESS;
		}
		case ASYNC_MPI_STATE_FAILURE: {
			PRINT_SES(ses);
			return ASYNC_FAILURE;
		}
		default: {
			printf("UNHANDLED CASE!\n");
			return ASYNC_FAILURE;
		}
	}
}
