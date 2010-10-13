#ifndef _list_h_
#define _list_h_

// -------------------------------------------------------------------------------
//
// List struct
//

typedef struct {
	void** data;
	uint16_t count;         // How many itemn are in the list
	uint16_t allocated;     // How much memory is allocated
} tList;

// -------------------------------------------------------------------------------
//
// List functions
//

tList* list_create(void);
void list_destroy(tList* list);

void list_status(tList* list);
void list_add(tList* list,void* item);
void list_remove(tList* list,void* item);

void list_swap(tList* list,uint16_t n1,uint16_t n2);
void list_shuffle(tList* list,const uint8_t* key, size_t keylen);

void list_foreach(tList* list,void* function);

#endif
