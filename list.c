#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "list.h"
#include "main.h"

#define LIST_SIZE_STEP 8
//~ #define sizeof(void*)

tList* list_create(void) {
	tList* list = malloc(sizeof(tList));
	memset(list,0,sizeof(tList));
	list->data = malloc(sizeof(void*) * LIST_SIZE_STEP);
	list->count = 0;
	list->allocated = LIST_SIZE_STEP;
	return list;
}

void list_destroy(tList* list) {
	free(list->data);
	free(list);
}

void* ListData(tList* list,uint16_t num) {
	return list->data[num];
}

void list_status(tList* list) {
	printf("count: %u/%u\n",list->count,list->allocated);
	int i;
	for (i=0;i<list->count;i++) {
		printf("%u: %p\n",i,list->data[i]);
	}
	//~ LogWordMemory("data: ",list->data,list->count);
}

void list_add(tList* list,void* data) {
	//~ printf("ListAdd(0x%04X)",data);
	if (list->count == list->allocated) {
		list->data=realloc(list->data, sizeof(void*) * (list->allocated + LIST_SIZE_STEP));
		list->allocated += LIST_SIZE_STEP;
	}
	list->data[list->count++]=data;
}

void list_remove(tList* list,void* data) {
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

void list_foreach(tList* list,void* function) {
	trace();
	int i;
	void (*f)(tList*,void*);
	f=function;
	for (i=0;i<list->count;i++) {
	trace();
		(*f)(list,list->data[i]);
	trace();
	}
	trace();
}

void ListSwap(tList* list,uint16_t n1,uint16_t n2) {
	void* tmp=list->data[n1];
	list->data[n1] = list->data[n2];
	list->data[n2] = tmp;
}

void ListShuffle(tList* list,const uint8_t* key, size_t keylen) {
	int16_t i;
	static uint16_t j=0;
	int16_t count=list->count;
	for(i=0; i<count; i++) {
		j = j + key[i%keylen] + 256*key[keylen-i%keylen];
		ListSwap(list,i,j%count);
	}
}
