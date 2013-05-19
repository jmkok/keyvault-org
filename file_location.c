#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "file_location.h"
#include "ssh.h"

// ---------------------------------------------------------------------
//
// Convert a text into an enum
//

enum FL_PROTO text_to_proto(const char* text) {
	if (strcmp(text,"local") == 0)
		return PROTO_FILE;
	if (strcmp(text,"ssh") == 0)
		return PROTO_SSH;
	return PROTO_UNKNOWN;
}

// ---------------------------------------------------------------------
//
// Read the data from the FILE LOCATION
//

extern int read_data(struct FILE_LOCATION* loc, void** data, ssize_t* length) {
	switch(loc->protocol) {
		case PROTO_UNKNOWN:
			assert(0);
		case PROTO_FILE:
			assert(0);
		case PROTO_SSH:
			return ssh_get_file(loc, data, length);
	};
	return -1;
}

// ---------------------------------------------------------------------
//
// Write the data to the FILE LOCATION
//

extern int write_data(struct FILE_LOCATION* loc, void* data, ssize_t length) {
	switch(loc->protocol) {
		case PROTO_UNKNOWN:
			assert(0);
		case PROTO_FILE:
			assert(0);
		case PROTO_SSH:
			return ssh_put_file(loc, data, length);
	};
	return -1;
}
