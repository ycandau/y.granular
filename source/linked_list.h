#ifndef YC_LINKED_LIST_H_
#define YC_LINKED_LIST_H_

// ======== DESCRIPTION ======== //
// Simple linked list implementation, using one array of integers

// ========  HEADER FILE FOR MISCELLANEOUS MAX UTILITIES  ========

#include "ext.h"      // Header file for all objects, should always be first
#include "ext_obex.h" // Header file for all objects, required for new style Max object

// ========  DEFINES  ========

#define LIST_END       -1
#define LIST_ERR_FULL  -2
#define LIST_ERR_EMPTY -3
#define LIST_ERR_END   -4
#define LIST_ERR_BEGIN -5
#define LIST_ERR_ARG   -6
#define LIST_NOT_FOUND -7

#define LIST_SAFE

// ====  GLOBAL VARIABLES  ====

typedef struct _list {

  t_int16  len;
  t_int16* first_used;
  t_int16* first_empty;
  t_int16* array;

} t_list;

// ====  PROCEDURE DECLARATIONS  ====

t_list*  list_new             (t_int16 n);
void     list_free            (t_list* list);

t_int16* list_prev_node       (t_list* list, t_int16* node);  // Decrement the node to the previous node in the list

void     list_insert_all      (t_list* list);                 // Insert all nodes in the list
t_int16  list_insert_last     (t_list* list);                 // Insert a node at the end of the list
t_int16  list_insert_nth      (t_list* list, t_int16 n);      // Insert a node before the nth node in the list
t_int16  list_insert_index    (t_list* list, t_int16 index);  // Look for node with specific index and insert it

void     list_remove_all      (t_list* list);                 // Remove all nodes from the list
t_int16  list_remove_last     (t_list* list);                 // Remove the node at the end of the list
t_int16  list_remove_nth      (t_list* list, t_int16 n);      // Remove the node in the nth position
t_int16  list_remove_index    (t_list* list, t_int16 index);  // Remove the node in the middle of the list

void     list_post  (void* x, t_list* list);

// ========  INLINE FUNCTIONS  ========

// ====  PROCEDURE: LIST_NEXT_NODE  ====
// Increment the node to the next node in the list
// RETURNS: The next node
// FAST: No looping through the lists

__inline t_int16* list_next_node(t_list* list, t_int16* node) {

  // If the node is already the last one, return the same node
#ifdef LIST_SAFE
  if (*node == LIST_END) { return node; }
#endif

  return (list->array + *node);
}

// ====  PROCEDURE: LIST_INSERT_FIRST  ====
// Take the first empty node and insert it at the beginning of the used list
// RETURNS: The index of the node just inserted
// FAST: No looping through the lists

__inline t_int16 list_insert_first(t_list* list) {

  // If no empty nodes are available, return an error
#ifdef LIST_SAFE
  if (*list->first_empty == LIST_END) { return LIST_ERR_FULL; }
#endif

  t_int16 tmp = *list->first_empty;
  *list->first_empty = list->array[tmp];
  list->array[tmp] = *list->first_used;
  *list->first_used = tmp;

  return tmp;
}

// ====  PROCEDURE: LIST_INSERT_NODE  ====
// Take the first empty node and insert it before the current node in the used list
// RETURNS: The index of the node just inserted
// FAST: No looping through the lists

__inline t_int16 list_insert_node(t_list* list, t_int16* node) {

  // If no empty nodes are available, return an error
#ifdef LIST_SAFE
  if (*list->first_empty == LIST_END) { return LIST_ERR_FULL; }
#endif

  t_int16 tmp = *list->first_empty;
  *list->first_empty = list->array[tmp];
  list->array[tmp] = *node;
  *node = tmp;

  return tmp;
}

// ====  PROCEDURE: LIST_REMOVE_FIRST  ====
// Remove the first node from the used list
// RETURNS: The index of the node just removed
// FAST: No looping through the lists

__inline t_int16 list_remove_first(t_list* list) {

  // If the used list is already empty, return an error
#ifdef LIST_SAFE
  if (*list->first_used == LIST_END) { return LIST_ERR_EMPTY; }
#endif

  t_int16 tmp = *list->first_used;
  *list->first_used = list->array[tmp];
  list->array[tmp] = *list->first_empty;
  *list->first_empty = tmp;

  return tmp;
}

// ====  PROCEDURE: LIST_REMOVE_NODE  ====
// Remove the next node from the used list
// RETURNS: The index of the node just removed
// FAST: No looping through the lists

__inline t_int16 list_remove_node(t_list* list, t_int16* node) {

  // If the used list is already empty, return an error
#ifdef LIST_SAFE
  if (*list->first_used == LIST_END) { return LIST_ERR_EMPTY; }
#endif

  t_int16 tmp = *node;
  *node = list->array[tmp];
  list->array[tmp] = *list->first_empty;
  *list->first_empty = tmp;

  return tmp;
}

// ========  END OF HEADER FILE  ========

#endif
