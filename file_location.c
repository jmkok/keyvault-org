#include <stdint.h>
#include <string.h>

#include "file_location.h"

enum FL_PROTO text_to_proto(const char* text) {
	if (strcmp(text,"local") == 0)
		return PROTO_FILE;
	if (strcmp(text,"ssh") == 0)
		return PROTO_SSH;
	return PROTO_UNKNOWN;
}
