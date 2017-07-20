/*
 * list.h -- doubly linked list implementation
 */
/*
 * Copyright (C) 2017 Eggheads Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* Prototypes (where N is programmer-supplied name, T the type)
 *
 * Types (don't assume their content, treat as opaque):
 *
 * struct list_N;
 * struct list_N_elem;
 *
 * Functions:
 *
 * Destroy a list
 * list_N_destroy(struct list_N *list);
 *
 * Create a list (allocates with nmalloc), freefunc can be NULL
 * struct list_N *list_N_create(void (*freefunc)(T data));
 *
 * Iterate over the list elements (_SAFE is safe against deletion)
 * list_N_foreach(struct list_N *list, void (*iter)(struct list_N_elem *elem));
 * LIST_FOREACH(listptr)
 * LIST_FOREACH_SAFE(listptr, struct list_N_elem *var, *var2)
 *
 * Iterate over the list elements' data
 * list_N_foreach_data(struct list_N *list, void (*iter)(T data));
 * LIST_FOREACH_DATA(listptr, struct list_N_elem *var, T datavar)
 */

#ifndef LIST_H
#define LIST_H

#define JOIN2(a, b) a ## b
#define JOIN3(a, b, c) a ## b ## c
#define EVALJOIN2(a, b) JOIN2(a, b)
#define EVALJOIN3(a, b, c) JOIN3(a, b, c)

#define LIST_NAME(name) JOIN2(list_, name)
#define LIST_ELEM_NAME(name) EVALJOIN2(LIST_NAME(name), _elem)

#define LIST_TYPE(name) struct LIST_NAME(name)
#define LIST_ELEM_TYPE(name) struct LIST_ELEM_NAME(name)
#define LIST_FUNC_NAME(name, func) EVALJOIN3(LIST_NAME(name), _, func)

#define LIST_ELEM_DATA(elem) ((elem)->data)

#define LIST_FOREACH(list, var)                                                \
  for ((var) = (list)->head; (var); (var) = (var)->next)

#define LIST_FOREACH_SAFE(list, var, var2)                                     \
  for ((var) = (list)->head;                                                   \
    (var) && (((var2) = (var)->next), 1);                                      \
    (var) = (var2))

#define LIST_FOREACH_DATA(list, var, dvar)                                     \
  for ((var) = (list)->head;                                                   \
    (var) && ((dvar) = LIST_ELEM_DATA(var), 1);                                \
    (var) = (var)->next)

#define CREATE_LIST_TYPE(name, datatype)                                       \
  LIST_ELEM_TYPE(name);                                                        \
  LIST_TYPE(name) {                                                            \
    int size;                                                                  \
    LIST_ELEM_TYPE(name) *head, *tail;                                         \
    void (*free)(datatype data);                                               \
  };                                                                           \
  LIST_ELEM_TYPE(name) {                                                       \
    LIST_ELEM_TYPE(name) *prev, *next;                                         \
    datatype data;                                                             \
  };                                                                           \
  void LIST_FUNC_NAME(name, foreach) (LIST_TYPE(name) *list,                   \
      void (*iter)(LIST_ELEM_TYPE(name) *elem)) {                              \
    LIST_ELEM_TYPE(name) *e;                                                   \
    for (e = list->head; e; e = e->next)                                       \
      iter(e);                                                                 \
  }                                                                            \
  void LIST_FUNC_NAME(name, foreach_safe) (LIST_TYPE(name) *list,              \
      void (*iter)(LIST_ELEM_TYPE(name) *elem)) {                              \
    LIST_ELEM_TYPE(name) *e, *next;                                            \
    for (e = list->head; e && (next = e->next, 1); e = next)                   \
      iter(e);                                                                 \
  }                                                                            \
  void LIST_FUNC_NAME(name, foreach_data) (LIST_TYPE(name) *list,              \
      void (*iter)(datatype data)) {                                           \
    LIST_ELEM_TYPE(name) *e;                                                   \
    for (e = list->head; e; e = e->next)                                       \
      iter(e->data);                                                           \
  }                                                                            \
  void LIST_FUNC_NAME(name, destroy) (LIST_TYPE(name) *list) {                 \
    if (list->free)                                                            \
      LIST_FUNC_NAME(name, foreach_data)(list, list->free);                    \
    nfree(list);                                                               \
  }                                                                            \
  LIST_TYPE(name) *LIST_FUNC_NAME(name, create) (                              \
      void (*freefunc)(datatype data)) {                                       \
    LIST_TYPE(name) *list = nmalloc(sizeof *list);                             \
    list->size = 0;                                                            \
    list->head = NULL;                                                         \
    list->free = freefunc;                                                     \
    return list;                                                               \
  }                                                                            \
  void LIST_FUNC_NAME(name, delete) (LIST_TYPE(name) *list,                    \
      LIST_ELEM_TYPE(name) *elem) {                                            \
    if (elem->prev)                                                            \
      elem->prev->next = elem->next;                                           \
    else                                                                       \
      list->head = elem->next;                                                 \
    if (elem->next)                                                            \
      elem->next->prev = elem->prev;                                           \
    else                                                                       \
      list->tail = elem->prev;                                                 \
    if (list->free)                                                            \
      list->free(elem->data);                                                  \
    list->size--;                                                              \
  }                                                                            \
  void LIST_FUNC_NAME(name, append) (LIST_TYPE(name) *list, datatype data) {   \
    LIST_ELEM_TYPE(name) *elem = nmalloc(sizeof *elem);                        \
    elem->data = data;                                                         \
    elem->next = NULL;                                                         \
    if (!list->head) {                                                         \
      list->head = elem;                                                       \
      elem->prev = NULL;                                                       \
    } else {                                                                   \
      elem->prev = list->tail;                                                 \
    }                                                                          \
    list->tail = elem;                                                         \
    list->size++;                                                              \
  }                                                                            \
  void LIST_FUNC_NAME(name, prepend) (LIST_TYPE(name) *list, datatype data) {  \
    LIST_ELEM_TYPE(name) *elem = nmalloc(sizeof *elem);                        \
    elem->data = data;                                                         \
    elem->prev = NULL;                                                         \
    if (!list->tail) {                                                         \
      list->tail = elem;                                                       \
      elem->next = NULL;                                                       \
    } else {                                                                   \
      elem->next = list->head;                                                 \
    }                                                                          \
    list->head = elem;                                                         \
    list->size++;                                                              \
  }                                                                            

#endif /* LIST_H */
