# C-style intrusive containers in C++

Intrusive containers is an implementation style of convential data storage structures 
(such as lists or trees) that embeds "stubs" of container-specific control data into
user data structures and then uses these stubs to "weave" user data into a container.

They are widely used in C code, including, for example, 
[Linux](https://github.com/torvalds/linux/blob/master/include/linux/list.h), 
[Windows](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/singly-and-doubly-linked-lists) and 
[BSD](https://www.freebsd.org/cgi/man.cgi?query=LIST_HEAD) kernels.

## C-style, in 4 steps

1\. Define the container structure (the "head"):

    struct list_head
    {
        struct list_item * first;
    };

2\. Define per-item data (the "item") that is used to place an user data item in a container:

    struct list_item
    {
        struct list_item * next;
    };
    
3\. Add an 'item' instance to the user data, one item for each container that 
this data can be on:

    struct user_data
    {
        ...
        struct list_item  vip_item;
        struct list_item  hip_item;
    }; 
    
4\. Use it:

    struct user_data  foo, bar;     // a couple of data items
    struct list_head  vip;          // "vip" list
    struct list_head  hip;          // "hip" list
    
    struct list_item * p;
    struct user_data * u;
    
    ...
    
    // populate lists
    list_add(&foo.vip_item, &vip);  // foo -> 'vip' list

    list_add(&bar.vip_item, &vip);  // bar -> 'vip' list
    list_add(&bar.hip_item, &hip);  // bar -> 'hip' list
    
    // traverse the 'vip' list
    for (p = vip.first; p; p = p->next)
        u = container_of(p, struct user_data, vip_item);
    
    // remove 'bar' from its lists
    list_del(&bar.vip_item);
    list_del(&bar.hip_item);
    
The magic ingredient that makes it all work is the [container_of](https://en.wikipedia.org/wiki/Offsetof#Usage)
macro. Given a pointer to a structure *field* it returns a pointer to the structure 
itself if we also provide it with the structure type and the field name.

## container_of

In the context of intrusive containers **container_of** is not the best name for this macro. 
It comes from the Linux kernel codebase and while technically correct (as it restores a pointer to
a struct that *contains* the field), a less ambiguous name would be **struct_of**.

Alternatively, FreeBSD and Windows kernels call it **CONTAINING_RECORD**, which is also
better, but a bit too mouthful.

With this caveat in mind and for the consistency sake the rest of the Readme will stick 
to `container_of`.

## Pros of intrusive containers

Adding items to a container requires no memory allocation. All required memory is effectively
preallocated by embedding container control items into the user data.

Items can be placed into multiple containers at once with all containers being on equal footing.
For example, items may be added to a list, to a ring, to a stack and to multiple maps, each keyed
by a different user data field, and with none of these containers having an "ownership" over the
item.

Item disposal and cleanup can be greatly simplified, assuming the container supports removal by 
control item as many of them do.

## Cons of intrusive containers

The main drawback is the risk of fundamentally messing things up.

One way to do it is with an incorrect `container_of` invocation:

    for (p = vip.first; p; p = p->next)             // traversing _vip_ list ...
        u = container_of(p, user_data, hip_item);   // ... but mistakenly recovering as a _hip_ list item
        ...

This will cause `u` to point at some random memory with all the consequences.

Another way is by adding an item to the wrong container of the same type:

    list_add(&bar.vip_item, &hip);  // should've been ".hip_item" ...
                                    // ... or "&vip"

Since all instances of a container share the same type, this will compile
happily, no questions asked.

## The same, in C++

When going from C to C++, the first thing to note is that it's not possible
to implement `offsetof`-based instrusive containers that will work in *all*
possible cases. This has to do with various edge and UB cases including, for 
example, virtual inheritance ([link](https://en.wikipedia.org/wiki/Offsetof#Limitations)).

However, for the C++ codebase that is already using C-style containers, it *is* 
possible to rework the code to safeguard against common pitfalls, while keeping 
the syntax very close to the original C style.

That is, **the goal** here is to make a better version of *C-style* containers
rather than to implement something *C++-style* and similar, for example, to 
what [Boost has](https://www.boost.org/doc/libs/1_64_0/doc/html/intrusive.html).

The resulting code looks like this:

    struct user_data
    {
        ...
        LIST_ITEM  vip;
        LIST_ITEM  hip;
    };

    CONTAINER_OF( user_data, vip );
    CONTAINER_OF( user_data, hip );

    void demo()
    {
        LIST_HEAD(user_data, vip)  vip_list;     // a list linked through user_data::vip
        LIST_HEAD(user_data, hip)  hip_list;     // a list linked through user_data::hip
    
        user_data  foo;                          // an instance of our data
        
        vip_list.add( &foo.vip );                // this will compile
        hip_list.add( &foo.hip );                // this will too
        
    //  vip_list.add( &foo.hip );                // but this will not, because item ...
                                                 // ... and head types are unrelated
        
        for (p = vip_list.first; p; p = p->next)
        {
            user_data * u = container_of(p);     // just need 'p' to recover user_data
            ...
        }
    }

Note how `container_of` now needs just a pointer to a `list_item`. In fact:

    user_data  foo;
    user_data  * p;
    
    p = container_of(&foo.vip_item);
    assert(p == &foo);
    
    p = container_of(&foo.hip_item);
    assert(p == &foo);
    
## How it's done

The idea is that we define a distinct `list_item` type for every `(user_data, field)`
combination and we also provide a version of `container_of()` to go with it. Like so:

    template <typename Instance>
    struct list_item
    {
        list_item<Instance> * next;
    };
    
    struct user_data
    {
        struct list_inst_1 { };               // unique dummy type #1
        list_item<list_inst_1>   vip_item;
        
        struct list_inst_2 { };               // unique dummy type #2
        list_item<list_inst_2>   hip_item;   
    };

Needing to create an unique dummy type for each field is a bit tiresome, 
so we wrap things into a macro that auto-generates all this for us based 
on the `__LINE__` value:

    #define LIST_ITEM          LIST_ITEM_1(__LINE__)
    #define LIST_ITEM_1(inst)  LIST_ITEM_2(inst)
    #define LIST_ITEM_2(inst)  struct list_inst_ ## inst { }; \
                               list_item<list_inst_ ## inst>

Next, we define a couple of macros to "recover" the exact `list_item` 
and `list_head` types from the containing struct and the field name:

    template <typename T> auto template_arg(list_item<T> &) -> T;

    #define LIST_HEAD_TYPE(T, field)  list_head<decltype(template_arg(T::field))>
    #define LIST_ITEM_TYPE(T, field)  list_item<decltype(template_arg(T::field))>

Finally, we provide a matching `container_of` version like so:

    #define CONTAINER_OF(T, field)                               \
        inline T * container_of(decltype(T::field) * item)       \
        {                                                        \
            return (T*)((char*)item - (int)offsetof(T, field));  \
        }

Note how this macro is not specific to `list_item` and works just as
fine for any container implemented along the same lines.

Now, if we go  back to the example from the previous section, the exact
type of `vip_item` in `user_data` will work out to something like 

    user_data::list_item<list_inst_4>
    
where 4 is the line number where `LIST_ITEM  vip_item;` is declared.

## The code

See [linked_list.h](linked_list.h) for the skeleton of a linked list container
and [linked_list_example.cpp](linked_list_example.cpp) for its sample usage.
