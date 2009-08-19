#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <rasqal.h>

#define SPACES_LENGTH 80
static const char spaces[SPACES_LENGTH+1]="																																								 ";

void* hx_parse_query ( unsigned char* query_string, unsigned char* base_uri_string );
