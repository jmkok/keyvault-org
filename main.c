#include "main.h"
#include "gtk_ui.h"
#include "gtk_dialogs.h"
#include "encryption.h"
#include "functions.h"

int main(int argc, char** argv)
{
	/* Initialize the gtk */
  gtk_init(&argc, &argv);

	/* Initialize the global setup */
	struct SETUP* setup = mallocz(sizeof(struct SETUP));

	/* Warn once about the beta status */
	if (!file_exists(".keyvault-warning.once")) {
		gtk_warning("This software is still in development\nKeep a plain text version of your passwords in a safe place\nThis warning will not be shown again");
		FILE* once = fopen(".keyvault-warning.once","w");
		if (once)
			fclose(once);
	}

  /* Initialize the random number genartor(do it here so we can warn the user with a dialog before doing anything else) */
  random_init();

	/* Initialize libxml2 */
	xmlThrDefIndentTreeOutput(1);
	xmlKeepBlanksDefault(0);

	/* Load any default file */
	setup->default_filename = NULL;
	int i;
	for (i=1;i<argc;i++) {
		setup->default_filename = argv[i];
	}

	/* Create the main window */
	create_main_window(setup);

  return 0;
}
