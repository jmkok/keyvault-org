#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <glib/gprintf.h>

#include "functions.h"

// -----------------------------------------------------------
//
// hexdump
//

//~ void hexdump(const void* ptr,int len) {
	//~ const unsigned char* data=ptr;
	//~ int i;
	//~ for (i=0;i<len;i++) {
		//~ printf("%02X ",data[i]);
	//~ }
//~ }

void hexdump(const void* ptr, int len) {
	volatile unsigned int i,j;
	const unsigned char* data = ptr;
	for (i=0;i<len;i+=16) {
		printf("0x%04zX:",(ssize_t)&data[i]-(ssize_t)ptr);
		for (j=0;j<16;j++) {
			if (j==8)
				printf(" ");
			if (i+j < len)
				printf(" %02X",data[i+j]);
			else
				printf("   ");
		}
		printf("  |");
		for (j=0;j<16;j++) {
			char c=data[i+j];
			if (c<0x20) c='.';
			if (c>=0x7f) c='.';
			if (i+j < len)
				printf("%c",c);
			else
				printf(" ");
		}
		printf("|\n\r");
	}
}

// -----------------------------------------------------------
//
// bin2hex
//

void bin2hex(char* dst,const char* src,int len) {
	int i;
	for (i=0;i<len;i++) {
		if (*dst) strcat(dst," ");			
		sprintf(dst+strlen(dst),"%02X",(unsigned char)src[i]);
	}
}

// -----------------------------------------------------------
//
// hex2bin
//

void hex2bin(char* dst,const char* src,int len) {
	//~ int i;
	//~ for (i=0;i<len;i++) {
		//~ if (*dst) strcat(dst," ");			
		//~ sprintf(dst+strlen(dst),"%02X",(unsigned char)src[i]);
	//~ }
}

// -----------------------------------------------------------
//
// concat 2 strings together
//

char* concat(char* s1,char* s2) {
	char* retval=malloc(strlen(s1)+strlen(s2)+1);
	strcpy(retval,s1);
	strcat(retval,s2);
	return retval;
}

// -----------------------------------------------------------
//
// convert a tring to lowercase
//

void lowercase(char string[]) {
   int  i = 0;
   while ( string[i] ) {
      string[i] = tolower(string[i]);
      i++;
   }
   return;
}

// -----------------------------------------------------------
//
// create a piece of zeroed memory
//

void* mallocz(int size) {
	void* ptr = malloc(size);
	memset(ptr,0,size);
	return ptr;
}

// -----------------------------------------------------------
//
// string trimming
//

void ltrim(char* src) {
	if (!src) return;
	char* dst=src;
	while ((*src != 0x00) && (*src <= 0x20))
		src++;
	if (src != dst)
		strcpy(dst,src);
}

void rtrim(char* src) {
	if (!src) return;
	int len=strlen(src);
	while ((len>0) && (src[len-1] <= 0x20))
		len--;
	src[len]=0;
}

void trim(char* src) {
	ltrim(src);
	rtrim(src);
}
