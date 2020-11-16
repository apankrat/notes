# Even more human-friendly intrusive containers in C++

This is a follow-up to the 
[previous installment](https://github.com/apankrat/notes/tree/master/intrusive-containers/readme-v1.md)
that showed how C-style "intrusive" containers can be adapted to C++,
adding compile-time protection against two types of mistakes, but 
without going full C++ on the syntax.

Second revision further reduces required boilerplate to a near-absolute
minimum. Here's an example of how it looks:

    /*
     *  Say we have our data structure and it needs to be on two
     *  separate lists - vip and hip.
     */
    struct user_data
    {
        list_item  vip;
        list_item  hip;
    };

`list_item` here is a macro expanding into a templated class instance,
so under the hood this creates two separate list_item types that are 
unrelated to each other.

Next, we can instantiate list heads of respective types like so:

    /*
     *  'vip_list' is a head of the list that is chained together
     *  through user_data::vip fields, and through these fields only!
     */
    list_head(user_data, vip)  vip_list;

    /*
     *  'hip_list' is similar, but using user_data::hip instead.
     */
    list_head(user_data, hip)  hip_list;

Next, we can populate the lists:

    /*
     *  Here's an instance of our data structure.
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

Finally, we can also enumerate items on a list and restore a pointer to
`user_data` from each item *unambiguously*:

    for (auto item = vip_list.first; item; item = item->next)
    {
        user_data * foo = container_of(p);
	...
    }

This is very close in readability to the C version, but also comes with 
compile-time checks against fat fingers. There's now no way to chain data
items by a wrong control field and it's not possible to mess up the 
container_of() invokation when recovering data pointer from a container
item.