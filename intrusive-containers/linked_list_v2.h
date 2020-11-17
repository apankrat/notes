/*
 *	https://github.com/apankrat/notes/blob/master/intrusive-containers
 *	C-style intrusive containers in C++
 */
#ifndef _LINKED_LIST_V2_H_
#define _LINKED_LIST_V2_H_

#include <stddef.h>

/* 
 *	linked list item
 */
template <typename T, int inst>
struct list_item
{
	list_item * next;
};

/*
 *	linked list head
 */
template <typename T, int inst>
struct list_head
{
	list_item<T, inst> * first;

	void add(list_item<T, inst> * item) { /* implement me */ }
};

/*
 *	Per-container boilerplate, not specific to 'list', covers:
 *
 *	1. Generating an unique 'item' type for the each 'cont_item' instance in 
 *	   a given struct.
 *	
 *	   #define __cont_item           __cont_item_1(__LINE__)
 *	   #define __cont_item_1(inst)   __cont_item_2(inst)
 *	   #define __cont_item_2(inst)   struct cont_inst_ ## id { }; \
 *	                                 cont_item<cont_inst_ ## id>
 *
 *	2. Recovering 'item' and 'head' types from the struct (T) that contains 
 *	   the head of a container as [field]
 *
 *	   template <typename T> auto template_arg(cont_item<T> &) -> T;
 *
 *	   #define __cont_head_type(T, field)  cont_head<decltype(template_arg(T::field))>
 *	   #define __cont_item_type(T, field)  cont_item<decltype(template_arg(T::field))>
 *
 *	3. Shorthand for cont_head, for consistency with cont_item
 *
 *	   #define __cont_head(T, field)  __cont_head_type(T, field)
 *
 *	4. CONTAINER_OF() instantiation for each container instance used
 */

// 1.
#define LIST_ITEM             LIST_ITEM_1(__LINE__)
#define LIST_ITEM_1(inst)     LIST_ITEM_2(inst)
#define LIST_ITEM_2(inst)     struct list_inst_ ## id { }; \
                              list_item<list_inst_ ## id>

// 2.
template <typename T> auto template_arg(list_item<T> &) -> T;

#define LIST_HEAD_TYPE(T, field)  list_head<decltype(template_arg(T::field))>
#define LIST_ITEM_TYPE(T, field)  list_item<decltype(template_arg(T::field))>

// 3.
#define LIST_HEAD(T, field)   LIST_HEAD_TYPE(T, field)

// 4.
#define CONTAINER_OF(T, field)                                      \
	inline T * container_of(decltype(T::field) * item)          \
	{                                                           \
		return (T*)((char*)item - (int)offsetof(T, field)); \
	}

#endif
