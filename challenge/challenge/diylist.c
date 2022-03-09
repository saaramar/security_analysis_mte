#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "diylist.h"

#include "mte_wrappers.h"

int fpool_num = 0;
FREEPOOL fpool[MAX_FREEPOOL] = {NULL};


/*
 * Create a new list
 */
List* list_new(void)
{
  List *list;

  list = (List*)my_malloc(sizeof(List));
  list->size = 0;
  list->max = 0;
  list->data = NULL;

  return list;
}

/*
 * Add an element
 */
void list_add(List* list, Data data, LIST_TYPE type)
{
  Data *p;
  
  if (list->size >= list->max) {
    /* Re-allocate a chunk if the list is full */
    Data *old = list->data;
    list->max += CHUNK_SIZE;
    
    list->data = (Data*)my_malloc(sizeof(Data) * list->max);
    if (list->data == NULL)
      __list_abort("Allocation error");

    if (old != NULL) {
      /* Copy and free the old chunk */

	    /*This is a bug in the original challenge*/
	    // my_memcpy((char*)list->data, (char*)old, sizeof(Data) * (list->max - 1));
      my_memcpy((char*)list->data, (char*)old, sizeof(Data) * (list->max - CHUNK_SIZE));
      my_free(old);
    }
  }

  /* Store the data */
  switch(type) {
  case LIST_LONG:
    list->data[list->size].d_long = data.d_long;
    break;
  case LIST_DOUBLE:
    list->data[list->size].d_double = data.d_double;
    break;
  case LIST_STRING:
    list->data[list->size].p_char = my_strdup(data.p_char);
    /* Insert the address to free pool */
    if (fpool_num < MAX_FREEPOOL) {
      fpool[fpool_num] = list->data[list->size].p_char;
      fpool_num++;
    }
    break;
  default:
    __list_abort("Invalid type");
  }
  
  list->size++;
}

/*
 * Get an element
 */
Data list_get(List* list, int index)
{
  if (index < 0 || list->size <= index)
    __list_abort("Out of bounds error");

  return (Data)list->data[index].p_char;
}

/*
 * Edit an element
 */
void list_edit(List* list, int index, Data data, LIST_TYPE type)
{
  if (index < 0 || list->size <= index)
    __list_abort("Out of bounds error");
  
  /* Store the data */
  switch(type) {
  case LIST_LONG:
    list->data[index].d_long = data.d_long;
    break;
  case LIST_DOUBLE:
    list->data[index].d_double = data.d_double;
    break;
  case LIST_STRING:
    list->data[index].p_char = my_strdup(data.p_char);
    /* Insert the address to free pool */
    if (fpool_num < MAX_FREEPOOL) {
      fpool[fpool_num] = list->data[list->size].p_char;
      fpool_num++;
    }
    break;
  default:
    __list_abort("Invalid type");
  }
}

/*
 * Delete an element
 */
void list_del(List* list, int index)
{
  int i;
  if (index < 0 || list->size <= index)
    __list_abort("Out of bounds error");

  Data data = list->data[index];

  /* Shift data list and remove the last one */
  for(i = index; i < list->size - 1; i++) {
    list->data[i] = list->data[i + 1];
  }
  list->data[i].d_long = 0;

  list->size--;

  /* Free data if it's in the pool list (which means it's string) */
  for(i = 0; i < fpool_num; i++) {
    if (fpool[i] == data.p_char) {
      my_free(data.p_char);
      break;
    }
  }
}

/*
 * Dump error and quit
 */
void __list_abort(const char *msg)
{
  fprintf(stderr, "[ABORT] %s\n", msg);
  exit(1);
}
