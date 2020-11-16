#ifndef _LINKED_LIST_V2_H_
#define _LINKED_LIST_V2_H_

#include <stddef.h>

/*
 *	boilerplate macros
 */
#define __container_item(T)   using cont_item_type = T;
#define __container_inst(id)  using cont_inst = char[id];

/* 
 *	linked list item
 */
template <typename T, int inst>
struct list_item
{
	// members
	list_item * next;

	// boilerplate, required
	__container_inst(inst);
};

/*
 *	linked list head
 */
template <typename T, int inst>
struct list_head
{
	// shorthand
	using list_item = list_item<T, inst>;

	// members
	list_item * first;

	// methods
	void add(list_item * item) { /* implement me */ }
};

/*
 *	Item and head type from the [T]ype that contains the list head container as [field]
 */
#define list_head_type(T, field)  list_head<T, sizeof(decltype(T::field)::cont_inst)>
#define list_item_type(T, field)  list_item<T, sizeof(decltype(T::field)::cont_inst)>

/*
 *	item type from the struct that is an item in a container
 */
#define list_item        list_item_1(__LINE__)
#define list_item_1(id)  list_item_2(id)
#define list_item_2(id)                 \
	static bool list_item_ ## id;   \
	list_item<cont_item_type, id>

/*
 *	shorthand
 */
#define list_head(T, field)  list_head_type(T, field)

/*
 *	how to restore a T pointer by a pointer to its [field]
 */
#define CONTAINER_OF(T, field)                                      \
	inline T * container_of(decltype(T::field) * item)          \
	{                                                           \
		return (T*)((char*)item - (int)offsetof(T, field)); \
	}

#endif
