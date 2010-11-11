#ifndef _configuration_h_
#define _configuration_h_

#include <stdint.h>
#include <gtk/gtk.h>
#include "list.h"
#include "structures.h"

// -----------------------------------------------------------
//
// variables
//

// -----------------------------------------------------------
//
// functions
//

extern void save_configuration(tList* kvo_list);
extern void read_configuration(tList* kvo_list);

tFileDescription* get_configuration(tConfigDescription* config, const unsigned char passphrase_key[32]);
void put_configuration(tFileDescription* kvo, const unsigned char passphrase_key[32]);

#endif
