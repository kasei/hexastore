/**
 * Jesse Weaver
 * Tetherless World Constellation
 * Rensselaer Polytechnic Institute
 */

#ifndef _ASYNC_H
#define _ASYNC_H

enum async_status {
	ASYNC_FAILURE = -1,
	ASYNC_PENDING = 0,
	ASYNC_SUCCESS = 1
};

typedef enum async_status (*async_t)(void* session);

#endif /* _ASYNC_H */
