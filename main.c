#include "ui.h"
#include "encryption.h"

extern int test_pbkdf2();

int main(int argc, char** argv)
{
	//~ return test_pbkdf2();
	
  gtk_init(&argc, &argv);
	create_main_window();
  return 0;
}
