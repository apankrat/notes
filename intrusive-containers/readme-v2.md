# Even more human-friendly intrusive containers in C++

The [original version](readme-v1.md)
described how C-style "intrusive" containers can be adapted to C++,
adding compile-time protection against two types of mistakes, but 
without going full C++ on the syntax.

This version takes it a bit further and reduces requried boilerplate 
to a near-absolute minimum.

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

`LIST_ITEM` here is a
[macro](linked_list_v2.h#L47-L51)
expanding into a
[templated class](linked_list_v2.h#L16)
instance. So under the hood this creates two distinct `list_item` types, one for each field.

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
every list item. And we do it by using *just* a `list_item` pointer,
nothing else:

    for (auto item = vip_list.first; item; item = item->next)
    {
        user_data * foo = container_of(p);
	...
    }

This is *very* close in style and expressivity to the C version, but it 
also has compile-time safe-guards against fat fingers. There's now no 
way to chain data items by a wrong control field nor is it possible 
to mess up the container_of() call when recovering `user_data` pointer
from a pointer to `list_item`.

# How it's done

See [linked_list_v2.h](linked_list_v2.h) and [linked_list_v2_example.cpp](linked_list_v2_example.cpp) for details.

