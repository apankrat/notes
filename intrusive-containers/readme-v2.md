# Even more human-friendly intrusive containers in C++

This is a follow-up to the 
[previous installment](https://github.com/apankrat/notes/tree/master/intrusive-containers/readme-v1.md)
that showed how C-style "intrusive" containers can be adapted to C++,
adding compile-time protection against two types of mistakes, but 
without going full C++ on the syntax.

This (second) revision further reduces the boilerplate to a near-absolute
minimum. Here's an example of how it looks:

    /*
     *  Say we have our data structure and it needs 
     *  to be on two separate lists - vip and hip.
     */
    struct user_data
    {
        list_item  vip;
        list_item  hip;
    };

`list_item` here is a macro expanding into a templated class instance,
so under the hood this creates two separate list_item types unrelated
to each other.

Next, we instantiate two respective lists:

    /*
     *  'vip_list' is a head of the list that is chained together
     *  through user_data::vip fields, and through these fields only.
     */
    list_head(user_data, vip)  vip_list;

    /*
     *  'hip_list' is similar, but it uses user_data::hip instead.
     */
    list_head(user_data, hip)  hip_list;

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
every list item. And we do it *unambiguously*:

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