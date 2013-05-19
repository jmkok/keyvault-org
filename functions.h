#ifndef _functions_h_
#define _functions_h_

#include "define.h"
#define free_if_defined(p) if(p) free(p)

extern void hexdump(const void* ptr, int len);
extern void bin2hex(char* dst,const char* src,int len);
extern char* concat(char* s1,char* s2);
extern void lowercase(char string[]);
extern void trim(char* src);
extern int file_exists(const char* filename);

#endif
