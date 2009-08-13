#include "util.h"

uint64_t hx_util_hash_string ( const char* s ) {
	uint64_t h	= 0;
	int len	= strlen(s);
	int i;
	for (i = 0; i < len; i++) {
		unsigned char ki	= (unsigned char) s[i];
		uint64_t highorder	= h & 0xfffffffff8000000;	// extract high-order 37 bits from h
		h	= h << 37;									// shift h left by 37 bits
		h	= h ^ (highorder >> 37);					// move the highorder 37 bits to the low-order end and XOR into h
		h = h ^ ki;										// XOR h and ki
	}
	return h;
}
