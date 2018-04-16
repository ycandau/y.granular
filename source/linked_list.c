// ========  TO DO  ========
// Safety checks
// Insert all and remove all
// Forward and backward version of linked list

#include "linked_list.h"

// ========  SIMPLE LINKED LIST STRUCTURE  ========

// ====  CONSTRUCTOR: LIST_NEW  ====
// Initializes a linked list which can hold up to n nodes.
// It uses an array of length (n+2), with two linked lists for used and empty cells
//   array[0] to array[n-2] are: array[i] = i+1
//   array[n-1] = LIST_END
//   array[n]   = LIST_END  and is the first used node
//   array[n+1] = 0         and is the first empty node
//   The used list is initially:  (END)
//   The empty list is initially: 0, 1, 2, ... n-1 (END)

t_list* list_new(t_int16 n) {

  t_list* list = (t_list*)sysmem_newptr(sizeof(t_list));
  if (list == NULL) { return NULL; }

  list->len   = n;
  list->array = (t_int16*)sysmem_newptr((list->len + 2) * sizeof(t_int16));

  list->first_used  = list->array + list->len;

  list->first_empty = list->array + list->len + 1;

  for (t_int16 i = 0; i < list->len - 1; i++) { list->array[i] = i + 1; }
  list->array[list->len - 1] = LIST_END;
  list->array[list->len]     = LIST_END;
  list->array[list->len + 1] = 0;

  return list;
}

// ====  DESTRUCTOR: LIST_FREE  ====
// Frees the memory allocated when the list was created

void list_free(t_list* list) {

  sysmem_freeptr(list->array);
  sysmem_freeptr(list);
}

// ====  PROCEDURE: LIST_PREV_NODE  ====
// Decrement the node to the previous node in the list
// RETURNS: The previous node
// SLOW: Loops through the used list to find the previous node

t_int16* list_prev_node(t_list* list, t_int16* node) {

  // If the node is already the first one, return the same node
  #ifdef LIST_SAFE
  if (node == list->first_used) { return node; }
  #endif

  t_int16* current = list->first_used;
  while (list->array + *current != node) { current = list->array + *current; }

  return (current);
}

// ====  PROCEDURE: LIST_INSERT_ALL  ====
// Take all empty nodes and insert them at the beginning of the used list
// RETURNS: Nothing
// SLOW: Loops through the empty list to find the last empty node

void list_insert_all(t_list* list) {

  ;
}

// ====  PROCEDURE: LIST_INSERT_LAST  ====
// Take the first empty node and insert it at the end of the used list
// RETURNS: The index of the node just inserted
// SLOW: Loops through the used list to find the last node

t_int16 list_insert_last(t_list* list) {

  // If no empty nodes are available, return an error
  #ifdef LIST_SAFE
  if (*list->first_empty == LIST_END) { return LIST_ERR_FULL; }
  #endif

  // Iterate through the used list to find the last node
  t_int16* node = list->first_used;
  while (*node != LIST_END) { node = list->array + *node; }

  *node              = *list->first_empty;
  *list->first_empty = list->array[*node];
  list->array[*node] = LIST_END;

  return* node;
}

// ====  PROCEDURE: LIST_INSERT_NTH  ====
// Take the first empty node and insert it before the nth node in the used list
// If there are less than n nodes in the used list the empty node is inserted at the end
// RETURNS: The index of the node just inserted
// SLOW: Loops through the used list to find the nth node

t_int16 list_insert_nth(t_list* list, t_int16 n) {

  // If no empty nodes are available, return an error
  #ifdef LIST_SAFE
  if (*list->first_empty == LIST_END) { return LIST_ERR_FULL; }
  #endif

  // Iterate through the used list to find the nth node
  t_int16 cnt = n;
  t_int16* node = list->first_used;
  while ((*node != LIST_END) && (cnt > 0)) { node = list->array + *node; cnt--; }

  t_int16 tmp        = *list->first_empty;
  *list->first_empty = list->array[tmp];
  list->array[tmp]   = *node;
  *node = tmp;

  return tmp;
}

// ====  PROCEDURE: LIST_INSERT_INDEX  ====
// Look for an index in the empty list and insert it at the beginning of the used list
// If the index is already in the used list do nothing
// RETURNS: The index of the node just inserted
// SLOW: Loops through the empty list to find the index

t_int16 list_insert_index(t_list* list, t_int16 index) {

  // Iterate through the empty list to find the index
  t_int16* node = list->first_empty;
  while ((*node != LIST_END) && (*node != index)) { node = list->array + *node; }

  // If the index is not found return LIST_NOT_FOUND
  if (*node == LIST_END) { return LIST_NOT_FOUND; }

  *node              = list->array[index];
  list->array[index] = *list->first_used;
  *list->first_used  = index;

  return index;
}

// ====  PROCEDURE: LIST_REMOVE_ALL  ====
// Take all used nodes and remove them
// RETURNS: Nothing
// SLOW: Loops through the used list to find the last used node

void list_remove_all(t_list* list) {

  ;
}

// ====  PROCEDURE: LIST_REMOVE_LAST  ====
// Remove the last node from the used list
// RETURNS: The index of the node just removed
// SLOW: Loops through the used list to find the last node

t_int16 list_remove_last(t_list* list) {

  // If the used list is already empty, return an error
  #ifdef LIST_SAFE
  if (*list->first_used == LIST_END) { return LIST_ERR_EMPTY; }
  #endif

  // Iterate through the used list to find the last node
  t_int16* node = list->first_used;
  t_int16* next = list->array + *node;
  while (*next != LIST_END) { node = next; next = list->array + *node; }

  list->array[*node] = *list->first_empty;
  *list->first_empty = *node;
  *node              = LIST_END;

  return* node;
}

// ====  PROCEDURE: LIST_REMOVE_NTH  ====
// Remove the nth node from the used list
// RETURNS: The index of the node just removed
// SLOW: Loops through the used list to find the nth node

t_int16 list_remove_nth(t_list* list, t_int16 n) {

  // If the used list is already empty, return an error
  #ifdef LIST_SAFE
  if (*list->first_used == LIST_END) { return LIST_ERR_EMPTY; }
  #endif

  // Iterate through the used list to find the nth node
  t_int16 cnt = n;
  t_int16* node = list->first_used;
  while ((*node != LIST_END) && (cnt > 0)) { node = list->array + *node; cnt--; }

  if (*node == LIST_END) { return LIST_ERR_ARG; }

  t_int16 tmp = *node;
  *node = list->array[tmp];
  list->array[tmp] = *list->first_empty;
  *list->first_empty = tmp;

  return tmp;
}

// ====  PROCEDURE: LIST_REMOVE_INDEX  ====
// Look for an index in the used list and remove it
// If the index is not in the used list do nothing
// RETURNS: The index of the node just removed
// SLOW: Loops through the used list to find the index

t_int16 list_remove_index(t_list* list, t_int16 index) {

  // Iterate through the used list to find the index
  t_int16* node = list->first_used;
  while ((*node != LIST_END) && (*node != index)) { node = list->array + *node; }

  // If the index is not found return LIST_NOT_FOUND
  if (*node == LIST_END) { return LIST_NOT_FOUND; }

  *node = list->array[index];
  list->array[index] = *list->first_empty;
  *list->first_empty = index;

  return index;
}

// ====  PROCEDURE: LIST_POST  ====
// Posts the current state of the list in the Max window

void list_post(void* x, t_list* list) {

  char tmp[10];

  t_int16  n_used = 0;
  t_int32  l_used = (t_int32)strlen("  Used list: ");
  t_int16* ptr = list->first_used;

  while (*ptr != LIST_END) {
    n_used++;
    sprintf(tmp, "%i ", *ptr);
    l_used += (t_int32)strlen(tmp);
    ptr = list->array + *ptr;
  }

  t_int16  n_empty = 0;
  t_int32  l_empty = (t_int32)strlen("  Empty list: ");

  ptr = list->first_empty;
  while (*ptr != LIST_END) {
    n_empty++;
    sprintf(tmp, "%i ", *ptr);
    l_empty += (t_int32)strlen(tmp);
    ptr = list->array + *ptr;
  }

  object_post((t_object*)x, "List length: %i - %i used - %i empty", n_used + n_empty, n_used, n_empty);

  char* str = (char*)sysmem_newptr((l_used + l_empty) * sizeof(char));

  strcpy(str, "  Used list: ");
  ptr = list->first_used;
  while (*ptr != LIST_END) {
    sprintf(tmp, "%i ", *ptr);
    strcat(str, tmp);
    ptr = list->array + *ptr;
  }

  object_post((t_object*)x, str);

  strcpy(str, "  Empty list: ");
  ptr = list->first_empty;
  while (*ptr != LIST_END) {
    sprintf(tmp, "%i ", *ptr);
    strcat(str, tmp);
    ptr = list->array + *ptr;
  }

  object_post((t_object*)x, str);

  sysmem_freeptr(str);
}
