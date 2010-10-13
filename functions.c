#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gprintf.h>

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

void hexdump(char* text, void* ptr, int len) {
	volatile unsigned int i,j;
	unsigned char* data=ptr;
	for (i=0;i<len;i+=16) {
		if (text)
			printf("%s:",text);
		else
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
// create a random password
//

gchar* create_random_password(int len) {
	char random_charset[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	gchar* password=g_malloc(len+1);
	memset(password,0,len+1);
	FILE* fp=fopen("/dev/urandom","r");
	int i,r;
	for (i=0;i<len;i++) {
		if (fp)
			fread(&r,1,sizeof(r),fp);
		else
			r=rand();
		char p=random_charset[r%strlen(random_charset)];
		password[i]=p;
	}
	if (fp)
		fclose(fp);
	return password;
}

