#ifndef _LINKED_LIST_H_
#define _LINKED_LIST_H_

#include <stddef.h>

/*
 *	Container templates
 */
template <typename T, const void * tag>
struct list_item
{
	list_item<T, tag> * next;
};

template <typename T, const void * tag>
struct list_head
{
	list_item<T, tag> * first;
};

/*
 *	Supporting macros
 */
#define LIST_ITEM(T, field)                   \
	static void * field ## _tag;          \
	list_item<T, &(field ## _tag)> field;

#define LIST_HEAD(T, field)                   \
	list_head<T, &T::field ## _tag>

#define CONTAINER_OF(T, field)                                      \
	inline T * container_of(decltype(T::field) * item)          \
	{                                                           \
		return (T*)((char*)item - (int)offsetof(T, field)); \
	}

/*
 *	Sample API
 */
template <typename T, const void * tag>
void list_add(list_item<T,tag> & item, list_head<T,tag> & list)
{
	// implement as needed
}

#endif
