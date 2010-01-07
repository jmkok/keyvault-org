#ifndef _functions_h_
#define _functions_h_

extern void hexdump(const char* text,const void* ptr,int len);
extern void bin2hex(char* dst,const char* src,int len);
extern char* concat(char* s1,char* s2);

#endif
