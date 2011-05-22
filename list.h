#ifndef _list_h_
#define _list_h_

// -------------------------------------------------------------------------------
//
// List struct
//

typedef struct LIST {
	void** data;
	uint16_t count;         // How many itemn are in the list
	uint16_t allocated;     // How much memory is allocated
	uint16_t _selected;
} tList;

// -------------------------------------------------------------------------------
//
// List functions
//

tList* listCreate(void);
void listDestroy(tList* list);

void listStatus(tList* list);
void listAdd(tList* list,void* item);
void listRemove(tList* list,void* item);

void listSwap(tList* list,uint16_t n1,uint16_t n2);
void listShuffle(tList* list,const uint8_t* key, size_t keylen);

//~ void listForeach(tList* list, void (*function)(tList*,void*));
tList* listExplode(const char* __src, const char* pattern);

#define listForeach(list,ptr) \
	for(list->_selected=0,ptr=list->data[0]; list->_selected < list->count; ptr=list->data[++list->_selected])

#endif
