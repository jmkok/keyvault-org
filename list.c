#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "list.h"
#include "functions.h"

#define LIST_SIZE_STEP 8
//~ #define sizeof(void*)

tList* listCreate(void) {
	tList* list = malloc(sizeof(tList));
	memset(list,0,sizeof(tList));
	list->data = malloc(sizeof(void*) * LIST_SIZE_STEP);
	list->count = 0;
	list->allocated = LIST_SIZE_STEP;
	return list;
}

void listDestroy(tList* list) {
	free(list->data);
	free(list);
}

void* listData(tList* list,uint16_t num) {
	return list->data[num];
}

void listStatus(tList* list) {
	printf("count: %u/%u\n",list->count,list->allocated);
	int i;
	for (i=0;i<list->count;i++) {
		printf("%u: %p\n",i,list->data[i]);
	}
	//~ LogWordMemory("data: ",list->data,list->count);
}

void listAdd(tList* list,void* data) {
	//~ printf("ListAdd(0x%04X)",data);
	if (list->count == list->allocated) {
		list->data=realloc(list->data, sizeof(void*) * (list->allocated + LIST_SIZE_STEP));
		list->allocated += LIST_SIZE_STEP;
	}
	list->data[list->count++]=data;
}

void listRemove(tList* list,void* data) {
	//~ printf("ListRemove(0x%04X)",data);
	int i;
	for (i=0;i<list->count;i++) {
		if (list->data[i] == data) {
			list->count--;
			memcpy(&(list->data[i]),&(list->data[i+1]),sizeof(void*) * (list->count-i));
			list->data[list->count]=NULL;
			break;
		}
	}
}

void listSwap(tList* list,uint16_t n1,uint16_t n2) {
	void* tmp=list->data[n1];
	list->data[n1] = list->data[n2];
	list->data[n2] = tmp;
}

void listShuffle(tList* list,const uint8_t* key, size_t keylen) {
	int16_t i;
	static uint16_t j=0;
	int16_t count=list->count;
	for(i=0; i<count; i++) {
		j = j + key[i%keylen] + 256*key[keylen-i%keylen];
		listSwap(list,i,j%count);
	}
}

tList* listExplode(const char* __src, const char* pattern) {
	tList* list = listCreate();
	char* tmp = strdup(__src);
	char* src = tmp;
	while(src && *src) {
		//~ printf(">> %s\n",src);
		char* ptr=strstr(src,pattern);
		if (ptr) {
			*ptr=0;
			ptr+=strlen(pattern);
		}
		trim(src);
		listAdd(list, strdup(src));
		src=ptr;
	}
	free(src);
	return list;
}
