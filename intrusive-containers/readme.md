# Human-friendly intrusive containers in C++

Intrusive containers is an implementation style of convential data storage structures 
(such as lists or trees) that uses  "stubs" of container-specific control data *embedded*
into application data and then uses these stubs to "weave" these data into a container.

# C-style

Define the container structure (the "head"):

    struct list_head
    {
        struct list_item * first;
    };

Define per-item data (the "item") that contains all container-specific data needed for
adding, finding, removing and otherwise working with a container item:

    struct list_item
    {
        struct list_item * next;
    };
    
Add an 'item' instance to the application data, one item for each container that this 
data can be on:

    struct user_data
    {
        ...
        struct list_item  vip_list;
        struct list_item  hip_list;
        struct list_item  hop_list;
    }; 
    
Use it:

    struct user_data  foo, bar, baz;
    struct list_head  vip, hip, hop;
    struct list_item * p;
    
    ...
    
    // populate lists
    list_add(&foo.vip_list, &vip);  // foo -> 'vip' list

    list_add(&bar.vip_list, &vip);  // bar -> 'vip' list
    list_add(&bar.hip_list, &hip);  // bar -> 'hip' list

    list_add(&baz.vip_list, &vip);  // baz -> 'vip' list
    list_add(&baz.hop_list, &hop);  // baz -> 'hop' list
    
    ...
    
    // traverse the 'vip' list
    for (p = vip.first; p; p = p->next)
    {
        struct user_data * u = container_of(p, struct user_data, vip_list);
        ...
    }
    
    ...
    
    // remove 'bar' from all lists
    list_del(&bar.vip_list);
    list_del(&bar.hip_list);
    list_del(&bar.hop_list);
    
The magic ingredient here is, of course, the [container_of](https://en.wikipedia.org/wiki/Offsetof#Usage) macro. 
Given a pointer to a structure *field* it allows restoring a pointer to the structure itself if told both the
structure type and the field name.

# Pros

Adding items to a container requires no memory allocation. All required memory is effectively
preallocated by embedding container control items into user data.

Items can be placed into multiple containers at once, e.g. on different maps, each keyed by a different  field 
of the user data.

Item disposal and cleanup is greatly simplified, assuming a container supports removal by control item (many do).

# Cons

The main drawback is the risk of inadvertent `container_of` misuse:

    for (p = vip.first; p; p = p->next)             // traversing _vip_ list ...
    {
        u = container_of(p, user_data, hip_list);   // ... but mistakenly recovering as a _hip_ list item
        ...

This will cause `u` to point at a random part of memory with all the consequences.

# C++-ized version

Due to the need for explicit casting in `container_of`, intrusive containers are not readily
compatible with C++. At least as far as improving code safety is concerned.

Boost has an [implementation](https://www.boost.org/doc/libs/1_64_0/doc/html/intrusive.html), 
but it's rather obvious 
from the [examples](https://www.boost.org/doc/libs/1_64_0/doc/html/intrusive/slist.html#intrusive.slist.slist_example)
how painful it is to fit round pegs in square holes.

There's however a middle-ground option, something that removes the risks of `container_of` mishandling while 
keeping code pretty close in readability to its C-style version:

    // types
    
    struct user_data
    {
        ...
        LIST_ITEM( user_data, vip );
        LIST_ITEM( user_data, hip );
        LIST_ITEM( user_data, hop );
    };

    LIST_ITEM_IMPL( user_data, vip );
    LIST_ITEM_IMPL( user_data, hip );
    LIST_ITEM_IMPL( user_data, hop );
    
    // vars
    
    user_data            foo;
    LIST_HEAD(foo, vip)  vip;           // vip list
    LIST_HEAD(foo, hip)  hip;           // hip list
    LIST_HEAD(foo, hop)  hop;           // hop list
    
    // usage
    
    list_add(&foo.vip_list, &vip);      // this compiles ...
    list_add(&foo.hip_list, &hop);      // .. and this doesn't
    
    for (p = vip.first; p; p = p->next)
    {
        user_data * u = container_of(p);
        ...
        
Note how `container_of` now needs just a pointer to `list_item`. In fact:

    user_data foo;
    user_data * p;
    
    p = container_of(&foo.vip);
    assert(p == &foo);
    
    p = container_of(&foo.hip);
    assert(p == &foo);

    p = container_of(&foo.hop);
    assert(p == &foo);
    
# How it's done

The idea is that we define a distinct `list_item` type for every
`(user_data, field)` pair and we also provide a version of
`container_of()` to go with it.

    template <typename T, const void * tag>
    struct list_item
    {
	      list_item<T, tag> * next;
    };

    #define LIST_ITEM(T, field)               \
        static void * field ## _tag;          \
        list_item<T, &(field ## _tag)> field;
        
The *trick* here is the use of `tag` template argument, which is 
set to the *address of static class member*, a value that is
guaranteed to be globally unique.

When used, `LIST_ITEM(foo, vip_list)` expands like so:

    ...
    static void * vip_list_tag;
    list_item<foo, &vip_list_tag> vip_list;
    ...
    
and creates a distinct `list_item` class just for the `vip_list` member.

The second part is to provide a matching `container_of` version and this is taken care of like so:

    #define LIST_ITEM_IMPL(T, field) \
        inline T * container_of(list_item<T, &T::field ## _anchor> & item) \
        {                                                                  \
		        return (T*)((char*)&item - (int)offsetof(T, field));           \
        }

And... that's it.
