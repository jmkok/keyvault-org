#ifndef _structures_h_
#define _structures_h_

#include "stdint.h"
#include <libxml/parser.h>

// -----------------------------------------------------------
//
// Global variables
//

typedef struct {
	char* title;						// title
	xmlNode* node;					// The node inside
	void* passphrase_check;	// The passphrase check
} tConfigDescription;

typedef struct {
	char* title;						// title
	char* protocol;		// protocol (local / ssh)
	char* filename;		// filename
	// network
	char* hostname;		// hostname (network protocol)
	uint16_t port;		// port (network protocol)
	char* username;		// username (network protocol)
	char* password;		// password (network protocol)
	// SSH specific...
	void* fingerprint;		// The SSH fingerprint
} tFileDescription;

typedef struct {
	void* data;
	int size;
} tData;

#endif
