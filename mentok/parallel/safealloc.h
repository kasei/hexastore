/**
 * Jesse Weaver
 * Tetherless World Constellation
 * Rensselaer Polytechnic Institute
 */

#ifndef __SAFEALLOC_H__
#define __SAFEALLOC_H__

#include <stdlib.h>

// Return values < 0 indicate that inappropriate use
// was avoided, but perhaps such a call was not as
// intended.
//
// Return values = 0 indicate failure of the call.
//
// Return values = 1 indicate typical success of the
// call.
//
// Return values > 0 indicate success of the call.
//
// Larger return values generally convey a sense of
// success that is less typical than smaller values.

#define SAFEALLOC_FAILED 0
#define SAFEALLOC_SUCCEEDED 1
#define SAFEALLOC_NEW_PTR 2
#define SAFEALLOC_ZERO_NULL 3
#define SAFEALLOC_ZERO_FREE 4
#define SAFEALLOC_NULL_FREED 5

int safe_malloc(void** ptr, size_t size); 
int safe_realloc(void** ptr1, void* ptr2, size_t size);
int safe_calloc(void** ptr, size_t nelem, size_t elsize);
int safe_free(void* ptr);

#endif /* __SAFEALLOC_H__ */
