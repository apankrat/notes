# Even more human-friendly intrusive containers in C++

This is a follow-up to the [original post](readme-v1.md) that described how C-style "intrusive" containers 
can be adapted to C++, adding compile-time protection against two types of mistakes, but without going full
C++ on the syntax.

This new, revised version takes it a step further and reduces requried boilerplate to a near-absolute minimum.

Here's an example of how it looks:

    /*
     *  Say we have our data structure and it needs 
     *  to be on two separate lists - vip and hip.
     */
    struct user_data
    {
        LIST_ITEM  vip;
        LIST_ITEM  hip;
    };

Next, we instantiate two respective lists:

    /*
     *  'vip_list' is a head of the list that is chained together
     *  through user_data::vip fields, and through these fields only.
     */
    LIST_HEAD(user_data, vip)  vip_list;

    /*
     *  'hip_list' is similar, but it uses user_data::hip instead.
     */
    LIST_HEAD(user_data, hip)  hip_list;

Next, we populate the lists:

    /*
     *  An instance of our data structure.
     */
    user_data foo;

    /*
     *  These two calls will compile fine ...
     */
    vip_list.add( &foo.vip );
    hip_list.add( &foo.hip );

    /*
     *  ... and these two won't, because head and item types are unrelated
     */
    vip_list.add( &foo.hip );
    hip_list.add( &foo.vip );

Finally, we traverse the list and restore a pointer to `user_data` from 
every list item. We do this by using *just* a `list_item` pointer and
nothing else:

    for (auto item = vip_list.first; item; item = item->next)
    {
        user_data * foo = container_of(p);
	...
    }

This is *very* close in style and expressivity to the C version, but it 
also has compile-time safeguards against fat fingers. There's now no 
way to chain data items by a wrong control field nor is it possible 
to mess up the container_of() call when recovering `user_data` pointer
from a pointer to `list_item`.

## How it's done

The gist of it is that `LIST_ITEM` is 
a [macro](linked_list_v2.h#L47-L51) expanding into 
a [templated class](linked_list_v2.h#L16) 
instance keyed by `__LINE__` and through that creating a distinct
`list_item` type for every `LIST_ITEM` field. It's a mouthful, but
it's simpler than it sounds.

See [original post](readme-v1.md) for the idea and [linked_list_v2.h](linked_list_v2.h) / [linked_list_v2_example.cpp](linked_list_v2_example.cpp) for details.

