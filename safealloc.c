/**
 * Jesse Weaver
 * Tetherless World Constellation
 * Rensselaer Polytechnic Institute
 */

#include "safealloc.h"

int safe_malloc(void** ptr, size_t size) {
	*ptr = malloc(size);
	if(size != 0) {
		if(*ptr != NULL) {
			return SAFEALLOC_SUCCEEDED;
		} else {
			return SAFEALLOC_FAILED;
		}
	} else {
		if(*ptr != NULL) {
			return SAFEALLOC_ZERO_FREE;
		} else {
			return SAFEALLOC_ZERO_NULL;
		}
	}
}

int safe_realloc(void** ptr1, void* ptr2, size_t size) {
	void* tmp = realloc(ptr2, size);
	int r;
	if(size != 0) {
		if(tmp != NULL) {
			if(tmp == ptr2) {
				r = SAFEALLOC_SUCCEEDED;
			} else {
				r = SAFEALLOC_NEW_PTR;
			}
		} else {
			r = SAFEALLOC_FAILED;
		}
	} else {
		if(tmp != NULL) {
			r = SAFEALLOC_ZERO_FREE;
		} else {
			r = SAFEALLOC_ZERO_NULL;
		}
	}
	*ptr1 = tmp;
	return r;
}

int safe_calloc(void** ptr, size_t nelem, size_t elsize) {
	*ptr = calloc(nelem, elsize);
	if(elsize != 0 && nelem != 0) {
		if(*ptr != NULL) {
			return SAFEALLOC_SUCCEEDED;
		} else {
			return SAFEALLOC_FAILED;
		}
	} else {
		if(*ptr != NULL) {
			return SAFEALLOC_ZERO_FREE;
		} else {
			return SAFEALLOC_ZERO_NULL;
		}
	}
}

int safe_free(void* ptr) {
	if(ptr != NULL) {
		free(ptr);
		return SAFEALLOC_SUCCEEDED;
	} else {
		return SAFEALLOC_NULL_FREED;
	}
}
