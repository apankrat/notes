#include "linked_list.h"
#include <assert.h>

/*
 *
 */
struct foo
{
	int something;

	LIST_ITEM(foo, vip_item);
	LIST_ITEM(foo, hip_item);
};

CONTAINER_OF_IMPL(foo, vip_item);
CONTAINER_OF_IMPL(foo, hip_item);

/*
 *
 */
void test()
{
	LIST_HEAD(foo, vip_item) vip_list;
	LIST_HEAD(foo, hip_item) hip_list;

	foo a, b;
	foo * p;

	/*
	 *	compiler-time checks
	 */
	list_add(a.vip_item, vip_list);  // these will compile OK
	list_add(a.hip_item, hip_list);

//	list_add(b.vip_item, hip_list);  // ...and these will not
//	list_add(b.hip_item, vip_list);

	/*
	 *	list traversal
	 */
	for (auto q = vip_list.first; q; q = q->next)
		container_of(q)->something++;

	/*
	 *	struct pointer recovery
	 */
	p = container_of(&a.vip_item);
	assert(p == &a);

	p = container_of(&b.hip_item);
	assert(p == &b);
}
