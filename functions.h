#ifndef _functions_h_
#define _functions_h_

#include <glib/gprintf.h>

#define trace() printf("<%s:%u>\n",__FILE__,__LINE__)
#define todo() printf("TODO <%s:%u>\n",__FILE__,__LINE__)

extern void hexdump(const char* text,const void* ptr,int len);
extern void bin2hex(char* dst,const char* src,int len);
extern char* concat(char* s1,char* s2);
extern void lowercase(char string[]);
extern void* mallocz(int size);

#endif
