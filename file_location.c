#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "functions.h"
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

const char* proto_to_text(enum FL_PROTO protocol) {
	switch(protocol) {
		case PROTO_UNKNOWN:
			return "unknown";
		case PROTO_FILE:
			return "local";
		case PROTO_SSH:
			return "ssh";
		default:
			return "unknown";
	};
}

// ---------------------------------------------------------------------
//
// Local file handlers
//

static int read_local_file(struct FILE_LOCATION* loc, void** data, ssize_t* length) {
	/* Get the stat for the file
	 * We need to allocate data before read */
	struct stat buf;
	int err = stat(loc->filename, &buf);
	if (err)
		return -1;

	/* Open the file */
	FILE* fp = fopen(loc->filename, "rb");
	if (!fp)
		return -1;

	*data = malloc(buf.st_size+1);
	*length = fread(*data, 1, buf.st_size, fp);
	fclose(fp);
	if (*length != buf.st_size)
		return -1;
	return 0;
}

static int write_local_file(struct FILE_LOCATION* loc, void* data, ssize_t length) {
	FILE* fp = fopen(loc->filename, "wb");
	if (!fp)
		return -1;
	ssize_t tx = fwrite(data, 1, length, fp);
	fclose(fp);
	if (tx != length)
		return -1;
	return 0;
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
			return read_local_file(loc, data, length);
		case PROTO_SSH:
			return ssh_get_file(loc, data, length);
		default:
			return -1;
	};
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
			return write_local_file(loc, data, length);
		case PROTO_SSH:
			return ssh_put_file(loc, data, length);
	};
	return -1;
}

// ---------------------------------------------------------------------
//
// Destroy a file location structure
//

void destroy_file_location(struct FILE_LOCATION* loc) {
	free_if_defined(loc->title);
	free_if_defined(loc->filename);
	free_if_defined(loc->hostname);
	free_if_defined(loc->username);
	free_if_defined(loc->password);
	free_if_defined(loc->fingerprint);
	free(loc);
}

// ---------------------------------------------------------------------
//
// Create a file location strcuture from an url
//

struct FILE_LOCATION* create_file_location_from_uri(const char* url) {
	printf("%s('%s')\n", __FUNCTION__, url);
	struct FILE_LOCATION* loc = calloc(1,sizeof(struct FILE_LOCATION));
	char* tmp = strdup(url);
	loc->title = strdup(url);

	/* Extract the protocol */
	char* next = strstr(tmp,"://");
	assert(next);
	*next = 0; next += 3;
	loc->protocol = text_to_proto(tmp);

	/* next points to the potential username
	 * extract the "username:password@hostname" */
	char* cur = next;
	next = strchr(next, '/');
	if (next) {
		*next = 0;
		next++;
	}

	/* Extract the username */
	char* ptr = strchr(cur,'@');
	if (ptr) {
		*ptr = 0;
		loc->username = strdup(cur);
		/* TODO: loc->password */
		cur = ptr+1;
	}
	else if (getenv("USER")) {
		loc->username = strdup(getenv("USER"));
	}

	/* Split the hostname:port */
	ptr = strchr(cur,':');
	if (ptr) {
		*ptr = 0;
		loc->port = atoi(ptr+1);
	}
	loc->hostname = strdup(cur);

	/* The filename is the remaining text */
	loc->filename = strdup(next);

	/* cleanup */
	free(tmp);
	return loc;
}
