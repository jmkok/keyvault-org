#ifndef _file_location_h_
#define _file_location_h_

// ---------------------------------------------------------------------
//
// A file location definition
//

struct FILE_LOCATION {
	char* title;			// title
	enum FL_PROTO {PROTO_UNKNOWN, PROTO_FILE, PROTO_SSH} protocol;		// protocol (local / ssh)
	char* filename;		// filename
	// network
	char* hostname;		// hostname (network protocol)
	uint16_t port;		// port (network protocol)
	char* username;		// username (network protocol)
	char* password;		// password (network protocol)
	// SSH specific...
	void* fingerprint;		// The SSH fingerprint
};

// ---------------------------------------------------------------------
//
// Functions
//

enum FL_PROTO text_to_proto(const char* text);
const char* proto_to_text(enum FL_PROTO);

void destroy_file_location(struct FILE_LOCATION* loc);
extern struct FILE_LOCATION* create_file_location_from_uri(const char* url);

extern int read_data(struct FILE_LOCATION*, void** data, ssize_t* length);
extern int write_data(struct FILE_LOCATION*, void* data, ssize_t length);

#endif /* _file_location_h_ */
