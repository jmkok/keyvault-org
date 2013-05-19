#ifndef _structures_h_
#define _structures_h_

#include "stdint.h"
#include "file_location.h"
#include <libxml/parser.h>

// ---------------------------------------------------------------------
//
// The global config
//

typedef struct {
	char* title;						// title
	xmlNode* node;					// The node inside
	void* passphrase_check;	// The passphrase check
} tConfigDescription;

// ---------------------------------------------------------------------
//
// A data structure
//

typedef struct {
	void* data;
	int size;
} tData;

#endif
