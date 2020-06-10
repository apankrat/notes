# Human-friendly intrusive containers in C++

Intrusive containers is an implementation style of convential data storage structures 
(such as lists or trees) that embeds "stubs" of container-specific control data into
application data structures and then uses "weaves" these stubs together to form a
container.

# C-style

Define the container structure (the "head"):

    struct list_head
    {
        struct list_item * first;
    };

Define per-item data (the "item") that is used to place an item in a container:

    struct list_item
    {
        struct list_item * next;
    };
    
Add an 'item' instance to the application data, one item for each container that 
this data can be on:

    struct user_data
    {
        ...
        struct list_item  vip_item;
        struct list_item  hip_item;
        struct list_item  hop_item;
    }; 
    
Use it:

    struct user_data  foo, bar, baz;
    struct list_head  vip, hip, hop;
    struct list_item * p;
    
    ...
    
    // populate lists
    list_add(&foo.vip_item, &vip);  // foo -> 'vip' list

    list_add(&bar.vip_item, &vip);  // bar -> 'vip' list
    list_add(&bar.hip_item, &hip);  // bar -> 'hip' list

    list_add(&baz.vip_item, &vip);  // baz -> 'vip' list
    list_add(&baz.hop_item, &hop);  // baz -> 'hop' list
    
    ...
    
    // traverse the 'vip' list
    for (p = vip.first; p; p = p->next)
    {
        struct user_data * u = container_of(p, struct user_data, vip_item);
        ...
    }
    
    ...
    
    // remove 'bar' from all lists
    list_del(&bar.vip_item);
    list_del(&bar.hip_item);
    list_del(&bar.hop_item);
    
The magic ingredient here is, of course, the [container_of](https://en.wikipedia.org/wiki/Offsetof#Usage) macro. 
Given a pointer to a structure *field* it allows restoring a pointer to the structure itself if told both the
structure type and the field name.

# Pros

Adding items to a container requires no memory allocation. All required memory is effectively
preallocated by embedding container control items into the user data.

Items can be placed into multiple containers at once, e.g. they can added to multiple maps,
each keyed by a different field of the user data.

Item disposal and cleanup is greatly simplified, assuming a container supports removal by 
control item (many do).

# Cons

The main drawback is the risk of inadvertent `container_of` misuse:

    for (p = vip.first; p; p = p->next)             // traversing _vip_ list ...
    {
        u = container_of(p, user_data, hip_item);   // ... but mistakenly recovering as a _hip_ list item
        ...

This will cause `u` to point at some random memory with all the consequences.

# C++-ized version

Due to the need for explicit casting in `container_of`, intrusive containers are not readily
compatible with C++. At least not as far as improving code safety is concerned.

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
        LIST_ITEM( user_data, vip_item );
        LIST_ITEM( user_data, hip_item );
    };

    CONTAINER_OF( user_data, vip_item );
    CONTAINER_OF( user_data, hip_item );

    // usage

    void test()
    {
        LIST_HEAD(user_data, vip_item) vip;  // vip list
        LIST_HEAD(user_data, hip_item) hip;  // hip list
    
        user_data  foo;
        
        list_add(&foo.vip_item, &vip);       // this compiles ...
        list_add(&foo.vip_item, &hip);       // .. and this doesn't
        
        for (p = vip.first; p; p = p->next)
        {
            user_data * u = container_of(p);
            ...
        }
    }

Note how `container_of` now needs just a pointer to `list_item`. In fact:

    user_data foo;
    user_data * p;
    
    p = container_of(&foo.vip);
    assert(p == &foo);
    
    p = container_of(&foo.hip);
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

When used, `LIST_ITEM(foo, vip_item)` expands like so:

    ...
    static void * vip_item_tag;
    list_item<foo, &vip_item_tag> vip_item;
    ...
    
and creates a distinct `list_item` class just for the `vip_list` member.

The second part is to provide a matching `container_of` version and this is taken care of like so:

    #define CONTAINER_OF(T, field)                               \
        inline T * container_of(decltype(T::field) * item)       \
        {                                                        \
            return (T*)((char*)item - (int)offsetof(T, field));  \
        }

Note how this macro is not specific to `list_item` in any way and works for any container 
implemented along the same lines.

And that's all there's to it.
