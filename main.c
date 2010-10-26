#include "gtk_ui.h"
#include "encryption.h"

int main(int argc, char** argv)
{
	// Initialize the gtk
  gtk_init(&argc, &argv);
  
  // Open the essential random handle (do it here so we can warn the user with a dialog before doing anything else
  random_init();

	// Create the main window
	create_main_window();

  return 0;
}
