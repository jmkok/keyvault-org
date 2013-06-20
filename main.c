#include <pwd.h>

#include "main.h"
#include "gtk_ui.h"
#include "gtk_dialogs.h"
#include "encryption.h"
#include "functions.h"

int main(int argc, char** argv)
{
	/* "Patch" unitiy with messing with "correct" position of the menu bar */
	setenv("UBUNTU_MENUPROXY",argv[0],1);

	/* chdir to the HOME directory */
	struct passwd* userInfoPtr = getpwuid(getuid());
	if (userInfoPtr)
		chdir(userInfoPtr->pw_dir);

	/* Initialize the gtk */
  gtk_init(&argc, &argv);

	/* Warn once about the beta status */
	if (!file_exists(FILE_WARN_ONCE)) {
		gtk_warning("This software is still in development\nKeep a plain text version of your passwords in a safe place\nThis warning will not be shown again");
		FILE* once = fopen(FILE_WARN_ONCE, "w");
		if (once)
			fclose(once);
	}

  /* Initialize the random number genartor
   * do it here so we can warn the user with a dialog before doing anything else */
  random_init();

	/* Initialize libxml2 */
	xmlThrDefIndentTreeOutput(1);
	xmlKeepBlanksDefault(0);

	/* Initialize the global setup */
	struct SETUP* setup = calloc(1,sizeof(struct SETUP));

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
