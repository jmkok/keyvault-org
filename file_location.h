#ifndef _file_location_h_
#define _file_location_h_

// ---------------------------------------------------------------------
//
// A file location definition
//

struct CONFIG;

struct FILE_LOCATION {
	/* generic */
	char* title;
	enum FL_PROTO {PROTO_UNKNOWN, PROTO_FILE, PROTO_SSH} protocol;
	char* filename;
	void* xml_node;
	/* network settings */
	char* hostname;
	uint16_t port;
	char* username;
	char* password;
	/* SSH settings */
	void* fingerprint;
};

// ---------------------------------------------------------------------
//
// Functions
//

enum FL_PROTO text_to_proto(const char* text);
const char* proto_to_text(enum FL_PROTO);

extern struct FILE_LOCATION* create_file_location(void);
extern struct FILE_LOCATION* create_file_location_from_uri(const char* url);
extern void destroy_file_location(struct FILE_LOCATION* loc);

extern struct FILE_LOCATION* get_file_location_by_index(struct CONFIG* config, int idx);
extern void store_file_location(struct CONFIG* config, struct FILE_LOCATION* loc);

extern int read_data(struct FILE_LOCATION*, void** data, ssize_t* length);
extern int write_data(struct FILE_LOCATION*, void* data, ssize_t length);

extern void fl_todo(struct FILE_LOCATION* loc);

#endif /* _file_location_h_ */
