#ifndef _ssh_h_
#define _ssh_h_

#include "structures.h"

extern int ssh_get_file(struct FILE_LOCATION*, void** data, ssize_t* length);
extern int ssh_put_file(struct FILE_LOCATION*, void* data, ssize_t length);
	
#endif
