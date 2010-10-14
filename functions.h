#ifndef _functions_h_
#define _functions_h_

#include <glib/gprintf.h>

extern void hexdump(const char* text,const void* ptr,int len);
extern void bin2hex(char* dst,const char* src,int len);
extern char* concat(char* s1,char* s2);
extern gchar* create_random_password(int len);
extern void lowercase(char string[]);

#endif
