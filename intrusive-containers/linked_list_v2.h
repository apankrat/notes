#ifndef _LINKED_LIST_V2_H_
#define _LINKED_LIST_V2_H_

#include <stddef.h>

/*
 *	boilerplate macros
 */
#define CONTAINER_ITEM(T)   using cont_item_type = T;
#define CONTAINER_INST(id)  using cont_inst = char[id];

/* 
 *	linked list item
 */
template <typename T, int inst>
struct list_item
{
	// members
	list_item * next;

	// boilerplate, required
	CONTAINER_INST(inst);
};

/*
 *	linked list head
 */
template <typename T, int inst>
struct list_head
{
	// members
	list_item<T, inst> * first;

	// methods
	void add(list_item<T, inst> * item) { /* implement me */ }
};

/*
 *	Item and head type from the [T]ype that contains the list head container as [field]
 */
#define LIST_HEAD_TYPE(T, field)  list_head<T, sizeof(decltype(T::field)::cont_inst)>
#define LIST_ITEM_TYPE(T, field)  list_item<T, sizeof(decltype(T::field)::cont_inst)>

/*
 *	item type from the struct that is an item in a container
 */
#define LIST_ITEM        LIST_ITEM_1(__LINE__)
#define LIST_ITEM_1(id)  LIST_ITEM_2(id)
#define LIST_ITEM_2(id)               \
	static bool list_item_ ## id; \
	list_item<cont_item_type, id>

/*
 *	shorthand
 */
#define LIST_HEAD(T, field)  LIST_HEAD_TYPE(T, field)

/*
 *	how to restore a T pointer by a pointer to its [field]
 */
#define CONTAINER_OF(T, field)                                      \
	inline T * container_of(decltype(T::field) * item)          \
	{                                                           \
		return (T*)((char*)item - (int)offsetof(T, field)); \
	}

#endif
