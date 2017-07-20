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

#define JOIN3(a, b, c) a ## b ## c
#define EVALJOIN3(a, b, c) JOIN3(a, b, c)

#define LIST_FUNC_NAME(name, func) JOIN3(name, _, func)

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

#define CREATE_LIST_TYPE(lname, ename, datatype)                               \
  struct ename;                                                                \
  struct lname {                                                               \
    int size;                                                                  \
    struct ename *head, *tail;                                                 \
    void (*freefunc)(datatype data);                                           \
  };                                                                           \
  struct ename {                                                               \
    struct ename *prev, *next;                                                 \
    datatype data;                                                             \
  };                                                                           \
  void LIST_FUNC_NAME(lname, foreach) (struct lname *list,                     \
      void (*iter)(struct ename *elem)) {                                      \
    struct ename *e;                                                           \
    for (e = list->head; e; e = e->next)                                       \
      iter(e);                                                                 \
  }                                                                            \
  void LIST_FUNC_NAME(lname, foreach_safe) (struct lname *list,                \
      void (*iter)(struct ename *elem)) {                                      \
    struct ename *e, *next;                                                    \
    for (e = list->head; e && (next = e->next, 1); e = next)                   \
      iter(e);                                                                 \
  }                                                                            \
  void LIST_FUNC_NAME(lname, foreach_data) (struct lname *list,                \
      void (*iter)(datatype data)) {                                           \
    struct ename *e;                                                           \
    for (e = list->head; e; e = e->next)                                       \
      iter(e->data);                                                           \
  }                                                                            \
  void LIST_FUNC_NAME(lname, destroy) (struct lname *list) {                   \
    if (list->freefunc)                                                        \
      LIST_FUNC_NAME(lname, foreach_data)(list, list->freefunc);               \
    nfree(list);                                                               \
  }                                                                            \
  struct lname *LIST_FUNC_NAME(lname, create) (                                \
      void (*freefunc)(datatype data)) {                                       \
    struct lname *list = nmalloc(sizeof *list);                                \
    list->size = 0;                                                            \
    list->head = NULL;                                                         \
    list->freefunc = freefunc;                                                 \
    return list;                                                               \
  }                                                                            \
  void LIST_FUNC_NAME(lname, delete) (struct lname *list,                      \
      struct ename *elem) {                                                    \
    if (elem->prev)                                                            \
      elem->prev->next = elem->next;                                           \
    else                                                                       \
      list->head = elem->next;                                                 \
    if (elem->next)                                                            \
      elem->next->prev = elem->prev;                                           \
    else                                                                       \
      list->tail = elem->prev;                                                 \
    if (list->freefunc)                                                        \
      list->freefunc(elem->data);                                              \
    list->size--;                                                              \
  }                                                                            \
  void LIST_FUNC_NAME(lname, append) (struct lname *list, datatype data) {     \
    struct ename *elem = nmalloc(sizeof *elem);                                \
    elem->data = data;                                                         \
    elem->next = NULL;                                                         \
    if (!list->head) {                                                         \
      list->head = elem;                                                       \
      elem->prev = NULL;                                                       \
    } else {                                                                   \
      elem->prev = list->tail;                                                 \
      elem->prev->next = elem;                                                 \
    }                                                                          \
    list->tail = elem;                                                         \
    list->size++;                                                              \
  }                                                                            \
  void LIST_FUNC_NAME(lname, prepend) (struct lname *list, datatype data) {    \
    struct ename *elem = nmalloc(sizeof *elem);                                \
    elem->data = data;                                                         \
    elem->prev = NULL;                                                         \
    if (!list->tail) {                                                         \
      list->tail = elem;                                                       \
      elem->next = NULL;                                                       \
    } else {                                                                   \
      elem->next = list->head;                                                 \
      elem->next->prev = elem;                                                 \
    }                                                                          \
    list->head = elem;                                                         \
    list->size++;                                                              \
  }                                                                            

#endif /* LIST_H */
