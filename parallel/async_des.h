/**
 * Jesse Weaver
 * Tetherless World Constellation
 * Rensselaer Polytechnic Institute
 */

#ifndef _ASYNC_DES_H
#define _ASYNC_DES_H

#include "parallel/async_mpi.h"
#include <stdlib.h>

typedef int (*async_des_handler)(async_mpi_session*, void*);

typedef struct {
	async_mpi_session** sends;
	async_mpi_session** recvs;
	size_t num_sends, num_recvs;
	int msg_count;
	async_des_handler send_handler, recv_handler;
	void* sendh_args, *recvh_args;
	int fixed_size;
	int tags;
	unsigned char first;
	unsigned char no_sends;
} async_des_session;

async_des_session* async_des_session_create(size_t num_sends, async_des_handler send_handler, void* sendh_args, size_t num_recvs, async_des_handler recv_handler, void* recvh_args, int fixed_size);

void async_des_session_destroy(async_des_session* ses);

enum async_status async_des(void* session);

#endif /* _ASYNC_DES_H */
