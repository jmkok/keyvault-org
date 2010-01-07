#ifndef _structures_h_
#define _structures_h_

// -----------------------------------------------------------
//
// Global variables
//

typedef struct {
	char* title;		// The user title
	char* protocol;	// which protocol (local / ssh)
	char* hostname;	// hostname (if server protocol)
	uint16_t port;	// portnumber (if server protocol)
	char* username;	// username (if server protocol)
	char* password;	// password (if server protocol)
	char* filename;	// filename
	// SSH specific...
	unsigned char* fingerprint;		// The SSH fingerprint
	// Where to store the remote file locally (required at this moment)
	char* local_filename;
	// Needed for descryption/encryption
	char* passphrase;
	unsigned char* ivec;
	int data_is_encrypted;
	int data_len;
	void* data;
} tKvoFile;

#endif
