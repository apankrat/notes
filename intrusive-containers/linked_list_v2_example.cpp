#include "linked_list_v2.h"
#include <assert.h>

/*
 *
 */
struct foo
{
	// boilerplate, required
	__container_item(foo);

	// members
	int something;

	list_item  vip;
	list_item  hip;
};

CONTAINER_OF(foo, vip);
CONTAINER_OF(foo, hip);

/*
 *
 */
void test()
{
	list_head(foo, vip)  vip_list;
	list_head(foo, hip)  hip_list;

	foo a, b;
	foo * p;

	/*
	 *	compiler-time checks
	 */
	vip_list.add(&a.vip);  // these will compile OK
	hip_list.add(&a.hip);

//	vip_list.add(&a.hip);  // .. and these will not
//	hip_list.add(&a.vip);

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
