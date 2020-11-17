/*
 *	https://github.com/apankrat/notes/blob/master/intrusive-containers
 *	C-style intrusive containers in C++
 */
#include "linked_list_v2.h"
#include <assert.h>

struct foo
{
	int  something;

	LIST_ITEM  vip;
	LIST_ITEM  hip;
};

/*
 *	instantiate container_of() for &foo::vip and &foo::hip
 */
CONTAINER_OF(foo, vip);
CONTAINER_OF(foo, hip);

void test()
{
	LIST_HEAD(foo, vip)  vip_list;
	LIST_HEAD(foo, hip)  hip_list;

	foo  a, b;
	foo  * p;

	/*
	 *	compile-time checks
	 */
	vip_list.add(&a.vip);  // these will compile ...
	hip_list.add(&a.hip);

//	vip_list.add(&a.hip);  // ... and these will not
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
