/**
 * Jesse Weaver
 * Tetherless World Constellation
 * Rensselaer Polytechnic Institute
 */

#include "parallel/async_des.h"
#include "parallel/safealloc.h"
#include <stdio.h>

#ifndef ASYNC_DES_CHECK_FREQ
#define ASYNC_DES_CHECK_FREQ 1
#endif

async_des_session* async_des_session_create(size_t num_sends, async_des_handler send_handler, void* sendh_args, size_t num_recvs, async_des_handler recv_handler, void* recvh_args, int fixed_size) {
	async_des_session* ses = NULL;
	if(safe_malloc((void**)&ses, sizeof(async_des_session)) > 0) {
		if(safe_malloc((void**)&(ses->sends), num_sends*sizeof(async_mpi_session*)) <= 0) {
			safe_free(ses);
			return NULL;
		}
		if(safe_malloc((void**)&(ses->recvs), num_recvs*sizeof(async_mpi_session*)) <= 0) {
			safe_free(ses->sends);
			safe_free(ses);
			return NULL;
		}
		size_t i;
		if(fixed_size > 0) {
			for(i = 0; i < num_sends; i++) {
				void* buf = NULL;
				int r = safe_malloc((void**)&buf, fixed_size);
				ses->sends[i] = async_mpi_session_create3(buf, fixed_size, -5, ASYNC_MPI_FLAG_IS_SENDING | ASYNC_MPI_FLAG_FREE_BUF | ASYNC_MPI_FLAG_FREE_REQUEST);
				if(r <= 0 || ses->sends[i] == NULL) {
					size_t j;
					for(j = 0; j < i; j++) {
						async_mpi_session_destroy(ses->sends[j]);
					}
					safe_free(ses->sends);
					safe_free(ses->recvs);
					safe_free(ses);
					return NULL;
				}
			}
			for(i = 0; i < num_recvs; i++) {
				void* buf = NULL;
				int r = safe_malloc((void**)&buf, fixed_size);
				ses->recvs[i] = async_mpi_session_create3(buf, fixed_size, MPI_ANY_SOURCE, ASYNC_MPI_FLAG_FREE_BUF | ASYNC_MPI_FLAG_FREE_REQUEST);
				ses->recvs[i]->tag = MPI_ANY_TAG;
				if(r <= 0 || ses->recvs[i] == NULL) {
					size_t j;
					for(j = 0; j < num_sends; j++) {
						async_mpi_session_destroy(ses->sends[j]);
					}
					for(j = 0; j < i; j++) {
						async_mpi_session_destroy(ses->recvs[j]);
					}
					safe_free(ses->sends);
					safe_free(ses->recvs);
					safe_free(ses);
					return NULL;
				}
				async_mpi(ses->recvs[i]);
			}
		} else {
			for(i = 0; i < num_sends; i++) {
				ses->sends[i] = async_mpi_session_create3(NULL, -1, -6, ASYNC_MPI_FLAG_IS_SENDING | ASYNC_MPI_FLAG_SHOULD_SEND_COUNT | ASYNC_MPI_FLAG_FREE_BUF | ASYNC_MPI_FLAG_FREE_REQUEST);
			}
			for(i = 0; i < num_recvs; i++) {
				ses->recvs[i] = async_mpi_session_create3(NULL, -1, MPI_ANY_SOURCE, ASYNC_MPI_FLAG_SHOULD_SEND_COUNT | ASYNC_MPI_FLAG_FREE_BUF | ASYNC_MPI_FLAG_FREE_REQUEST);
				async_mpi(ses->recvs[i]);
			}
		}
		ses->num_sends = num_sends;
		ses->num_recvs = num_recvs;
		ses->msg_count = 0;
		ses->send_handler = send_handler;
		ses->recv_handler = recv_handler;
		ses->sendh_args = sendh_args;
		ses->recvh_args = recvh_args;
		ses->fixed_size = fixed_size;
		ses->first = 1;
		ses->tags = 1;
		ses->no_sends = 0;
	}
	return ses;
}

void async_des_session_destroy(async_des_session* ses) {
	size_t i;
	for(i = 0; i < ses->num_sends; i++) {
		async_mpi_session_destroy(ses->sends[i]);
	}
	for(i = 0; i < ses->num_recvs; i++) {
		async_mpi_session_destroy(ses->recvs[i]);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	safe_free(ses->sends);
	safe_free(ses->recvs);
	safe_free(ses);
}

#define _CHECK_R(r) if(r == ASYNC_FAILURE) fprintf(stderr, "FAILURE in async_des!\n")
//#define _CHECK_R(r)
//#define DEBUG(...) fprintf(stdout, __VA_ARGS__)
#define DEBUG(...)
enum async_status async_des(void* session) {
	DEBUG("async_des\n");
	async_des_session* ses = (async_des_session*) session;
	size_t i, done;
	done = 0;
	int r;
	size_t max = ses->num_sends < ses->num_recvs ? ses->num_recvs : ses->num_sends;
	size_t j;
	for(j = 0; j < ASYNC_DES_CHECK_FREQ; j++) {
		for(i = 0; i < max; i++) {
			if(!ses->no_sends && i < ses->num_sends) {
				if(!ses->first) {
					r = async_mpi(ses->sends[i]);
					_CHECK_R(r);
				}
				if(ses->first || r != ASYNC_PENDING) {
				// while(r != ASYNC_PENDING) {
					// send_handler should reset only if there is something left
					if((*(ses->send_handler))(ses->sends[i], ses->sendh_args)) {
						ses->sends[i]->tag = ses->tags;
						ses->tags++;
						if(ses->tags < 1) {
							ses->tags = 1;
						}
						r = async_mpi(ses->sends[i]);
						_CHECK_R(r);
						ses->msg_count++;
						DEBUG("msg_count = %i\n", ses->msg_count);
					} else {
						r = ASYNC_PENDING;
						done++;
					}
				}
			}
			if(i < ses->num_recvs) {
				r = async_mpi(ses->recvs[i]);
				_CHECK_R(r);
				// printf("ses->recvs[%i]->state = %i\n", i, ses->recvs[i]->state);
				if(r != ASYNC_PENDING) {
				// while(r != ASYNC_PENDING) {
					ses->msg_count--;
					DEBUG("msg_count = %i\n", ses->msg_count);
					if(!(*(ses->recv_handler))(ses->recvs[i], ses->recvh_args)) {
						return ASYNC_FAILURE;
					}
					async_mpi_session_reset3(ses->recvs[i], ses->recvs[i]->buf, ses->recvs[i]->count, MPI_ANY_SOURCE, ses->recvs[i]->flags);
					if(ses->fixed_size > 0) {
						ses->recvs[i]->tag = MPI_ANY_TAG;
					}
					r = async_mpi(ses->recvs[i]);
					_CHECK_R(r);
				}
			}
		}
	}
	ses->first = 0;
	if(done >= ses->num_sends) {
		ses->no_sends = 1;
	}
	//if(ses->no_sends) {
	ses->msg_count = ses->no_sends ? ses->msg_count : ses->msg_count + 1;
	int total_msg_count;
	MPI_Allreduce(&(ses->msg_count), &total_msg_count, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
	if(ses->no_sends && total_msg_count == 0) {
		return ASYNC_SUCCESS;
	}
	ses->msg_count = ses->no_sends ? ses->msg_count : ses->msg_count - 1;
	//}
	return ASYNC_PENDING;
}
#undef _CHECK_R
