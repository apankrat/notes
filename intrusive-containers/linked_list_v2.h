/*
 *	https://github.com/apankrat/notes/blob/master/intrusive-containers
 *	C-style intrusive containers in C++
 */
#ifndef _LINKED_LIST_V2_H_
#define _LINKED_LIST_V2_H_

#include <stddef.h>

/* 
 *	Linked list item
 */
template <typename T, int inst>
struct list_item
{
	list_item * next;
};

/*
 *	Linked list head
 */
template <typename T, int inst>
struct list_head
{
	list_item<T, inst> * first;

	void add(list_item<T, inst> * item) { /* implement me */ }
};

/*
 *	The boilerplate:
 *
 *	1. To generate an unique 'item' type for the each 'list_item' 
 *	   instance in a holding struct.
 */
#define LIST_ITEM          LIST_ITEM_1(__LINE__)
#define LIST_ITEM_1(inst)  LIST_ITEM_2(inst)
#define LIST_ITEM_2(inst)  struct list_inst_ ## id { }; \
                           list_item<list_inst_ ## id>

/*
 *	2. To recover 'item' and 'head' types from a struct (T) that 
 *	   contains the head of a container as its [field]
 */
template <typename T>
auto template_arg(list_item<T> &) -> T;

#define LIST_HEAD_TYPE(T, field)  list_head<decltype(template_arg(T::field))>
#define LIST_ITEM_TYPE(T, field)  list_item<decltype(template_arg(T::field))>

/*
 *	3. Shorthand for LIST_HEAD, for consistency with LIST_ITEM
 */
#define LIST_HEAD(T, field)  LIST_HEAD_TYPE(T, field)

/*
 *	Finally, CONTAINER_OF() - a boilerplate macro that needs to
 *	be instantiated for every container instance defined. It is
 *	not list-specific. See sample code for details.
 */
#define CONTAINER_OF(T, field)                                      \
	inline T * container_of(decltype(T::field) * item)          \
	{                                                           \
		return (T*)((char*)item - (int)offsetof(T, field)); \
	}

#endif
