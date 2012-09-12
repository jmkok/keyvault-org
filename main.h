#ifndef _main_h_
#define _main_h_

#include <stdint.h>
#include <gtk/gtk.h>
#include "list.h"

// -----------------------------------------------------------
//
// variables
//

struct SETUP {
	struct CONFIG* config;
	const char* default_filename;
};

#endif
