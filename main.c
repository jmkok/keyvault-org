#include "gtk_ui.h"
#include "encryption.h"

int main(int argc, char** argv)
{
	// Initialize the gtk
  gtk_init(&argc, &argv);

  // Open the essential random handle (do it here so we can warn the user with a dialog before doing anything else
  random_init();

	// Initialize libxml2
	xmlThrDefIndentTreeOutput(1);
	xmlKeepBlanksDefault(0);
  
	// Load a default file
	char* filename = NULL;
	int i;
	for (i=1;i<argc;i++) {
		filename = argv[i];
	}

	// Create the main window
	create_main_window(filename);

  return 0;
}
