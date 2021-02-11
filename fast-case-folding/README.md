# Fast character case conversion

Converting strings and characters between lower and upper cases is a very 
common need.

In particular, case-insensitive string comparision is usually implemented 
this way - a comparison that often occurs on the program's fast paths as 
a part of data container lookups and content manipulation.

So it is usually desirable to make case conversions (and, by extension,
case-insensitive comparisions) as fast as possible.

In this post we are going to look at one of the options - very fast 
case conversion using **compressed lookup tables**.

If in rush, you can jump straight to the [Conclusion](#Conclusion).

## Lookup tables

The simplest way to case conversion is with a lookup table:

    const char to_lower[256] = { ... }; // lower case versions of every character
    const char to_upper[256] = { ... }; // upper case versions of every character
  
    char a = to_lower[ 'A' ];
    char Z = to_upper[ 'z' ];
    ...

This is also the fastest way. The cost of speed is the space needed to 
hold the lookup tables, the good old
[space-time tradeoff](https://en.wikipedia.org/wiki/Space%E2%80%93time_tradeoff).

If we are working with the 7-bit ASCII set, the tables will take 512 bytes.

These can be allocated and initialized statically (and therefore stored
in the executable binary) or they can be created at run-time. This is
doable because the coversion rules are trivial:

    if ('A' <= ch && ch <= 'Z') ch += 'a' - 'A'; // to lower
    if ('a' <= ch && ch <= 'z') ch += 'A' - 'a'; // to upper

In fact, for this case the benefits of using a lookup table are marginal 
at best. The above `if` construct could work just as well.

## 8-bit ASCII

Case conversion for an 8-bit ASCII set depends on the 
[locale](https://en.wikipedia.org/wiki/Locale_(computer_software))
used.

In terms of speeding things up the same applies as before - either
hardcode static conversion table for each locale of interest, or
code the conversion rules and then initialize the lookup tables at
run-time.

## Unicode

Unicode is where it gets complicated.

At the time of writing the [Unicode Standard](http://www.unicode.org/versions/latest/) 
is at its 13th revision. The principle list of characters is [UnicodeData.txt](https://www.unicode.org/Public/13.0.0/ucd/UnicodeData.txt)
with its format described [here](http://www.unicode.org/L2/L1999/UnicodeData.html).
The executive summary of this fascinating read is this:

* There are around 1400 characters with lower/upper cases
* Some have more than one lower/upper case
* Some use multiple characters for one case and a single character for another

For the purpose of this post we are going to ignore #2 and #3 and 
focus on characters that covert one-to-one between their cases.

We will also focus on conversion of characters from
`0x0000 - 0xD7FF` and `0xE000 - 0xFFFF` ranges, i.e. those that 
encode as a single 16-bit word under
[UTF-16](https://en.wikipedia.org/wiki/UTF-16) encoding.

## Unicode, continued

A lookup table for the UTF-16 case requires 2 x 65536 bytes per
case, 256KB in total.

Both tables will also be *very* sparse with just 2% of a non-trivial fill.

Which brings us to the **interesting part** - how can we compress these
tables without sacrificing the speed of lookups?

One of the answers can be found in the [Wine](https://winehq.org) project
or, more specifically, in its
[unicode.h](https://github.com/wine-mirror/wine/blob/e909986e6ea5ecd49b2b847f321ad89b2ae4f6f1/include/wine/unicode.h#L93).

Pared down a bit it looks like this -

    wchar_t tolower(wchar_t ch)
    {
        extern const wchar_t casemap_lower[];
        return ch + casemap_lower[casemap_lower[ch >> 8] + (ch & 0xff)];
    }

Two bit operations, two additions and two memory references.

The `casemap_lower` table lives in
[casemap.c](https://github.com/wine-mirror/wine/blob/e909986e6ea5ecd49b2b847f321ad89b2ae4f6f1/libs/port/casemap.c)
and it is quite incredibly just 8KB is size:

    const wchar_t wine_casemap_lower[4122] = { ... }

This is easily one of more elegant pieces of code I've seen in a long while. 

Very fast, beautifully terse and, at the first glance, mysterious. Here's how it works.

## Table compression

Both `tolower` and `toupper` tables are compressed the same way, so
we'll just look at the compression of the former.

There are 1158 conversion rules and they are listed [here](to-lower-table.txt).

So here's what's going on:

1. The first insight is to store **deltas** between the lower and 
   upper case codes rather than their absolute values.
  
   That is, we do `ch += lookup[...]` instead of `ch = lookup[...]`.
   
   This fills the table with 0s for all non-caseable characters, 
   creating *lots* of redundancy and making it way easier to compress.

2. Next, the full set of possible `wchar_t` values - all 65536 of
   them - is split into 256 blocks, 256 characters each. Entries 
   for values starting with 00xx (in hex) go into the first block, 
   01xx - into the second, etc.
   
       block 00 - [256 deltas for values 0000 to 00FF]
       block 01 - [256 deltas for values 0100 to 01FF]
        ...
       block FF - [256 deltas for values FF00 to FFFF]
    
3. Once this is done, we notice that only 17 blocks are **not** 
   completely zero-filled - 
   
       00, 01, 02, 03, 04, 05, 10, 13, 1C, 1E, 1F, 21, 24, 2C, A6, A7, FF
   
   Remaining 239 blocks will be all full of zeroes.
   
   So we can store just these 17 blocks plus a zero-filled block and 
   then use an *index*  to map block ID to its entry in the table:
   
       Index                     |   Table
       --------------------------+-------------------------------------------
       block     offset          |   offset    data
       00xx  ->  0000            |   0000      [... 256 deltas ...]
       01xx  ->  0100            |   0100      [... 256 deltas ...]
       ...                       |   0200      [... 256 deltas ...] 
       05xx  ->  0500            |   ...
       06xx  ->  1200 (zeroes)   |   1100      [... 256 deltas ...] 
       07xx  ->  1200 (zeroes)   |   1200      [ 0 0 0 0 0 0 0 ...]  (zeroes)
       ...                       |
       FExx  ->  1200 (zeroes)   |
       FFxx  ->  1100            |

    This compresses table down to **4608** items (18 x 256) + the 
    size of the index. This is already excellent - around 10K 
    instead of 128K - but it can be compressed further.
    
 4. Another insight is that remaining blocks still contain a lot
    of zeroes. More specifically, one block may *end* with lots 
    of zeroes and the next onemay *start* with a lot of them.
    
    Here's, for example, the end of the 0200 block -
    
        ...
        0248 | 0001 0000 0001 0000 0001 0000 0001 0000  <- last non-zeros
        0250 | 0000 0000 0000 0000 0000 0000 0000 0000
        0258 | 0000 0000 0000 0000 0000 0000 0000 0000
        0260 | 0000 0000 0000 0000 0000 0000 0000 0000
        0268 | 0000 0000 0000 0000 0000 0000 0000 0000
        0270 | 0000 0000 0000 0000 0000 0000 0000 0000
        0278 | 0000 0000 0000 0000 0000 0000 0000 0000
        0280 | 0000 0000 0000 0000 0000 0000 0000 0000
        0288 | 0000 0000 0000 0000 0000 0000 0000 0000
        0290 | 0000 0000 0000 0000 0000 0000 0000 0000
        0298 | 0000 0000 0000 0000 0000 0000 0000 0000
        02a0 | 0000 0000 0000 0000 0000 0000 0000 0000
        02a8 | 0000 0000 0000 0000 0000 0000 0000 0000
        02b0 | 0000 0000 0000 0000 0000 0000 0000 0000
        02b8 | 0000 0000 0000 0000 0000 0000 0000 0000
        02c0 | 0000 0000 0000 0000 0000 0000 0000 0000
        02c8 | 0000 0000 0000 0000 0000 0000 0000 0000
        02d0 | 0000 0000 0000 0000 0000 0000 0000 0000
        02d8 | 0000 0000 0000 0000 0000 0000 0000 0000
        02e0 | 0000 0000 0000 0000 0000 0000 0000 0000
        02e8 | 0000 0000 0000 0000 0000 0000 0000 0000
        02f0 | 0000 0000 0000 0000 0000 0000 0000 0000
        02f8 | 0000 0000 0000 0000 0000 0000 0000 0000
       
    And here's the start of the 0300 one -
    
        0300 | 0000 0000 0000 0000 0000 0000 0000 0000
        0308 | 0000 0000 0000 0000 0000 0000 0000 0000
        0310 | 0000 0000 0000 0000 0000 0000 0000 0000
        0318 | 0000 0000 0000 0000 0000 0000 0000 0000
        0320 | 0000 0000 0000 0000 0000 0000 0000 0000
        0328 | 0000 0000 0000 0000 0000 0000 0000 0000
        0330 | 0000 0000 0000 0000 0000 0000 0000 0000
        0338 | 0000 0000 0000 0000 0000 0000 0000 0000
        0340 | 0000 0000 0000 0000 0000 0000 0000 0000
        0348 | 0000 0000 0000 0000 0000 0000 0000 0000
        0350 | 0000 0000 0000 0000 0000 0000 0000 0000
        0358 | 0000 0000 0000 0000 0000 0000 0000 0000
        0360 | 0000 0000 0000 0000 0000 0000 0000 0000
        0368 | 0000 0000 0000 0000 0000 0000 0000 0000
        0370 | 0001 0000 0001 0000 0000 0000 0001 0000  <- first non-zeros
        ...
 
    So the 0300 block can be "pulled up" to overlap with
    the 0200 block and its index entry adjusted to match.
    
    The blocks can be **squished** together if you will.
    Like so -
    
        ...
        0248 | 0001 0000 0001 0000 0001 0000 0001 0000
        0250 | 0000 0000 0000 0000 0000 0000 0000 0000
        0258 | 0000 0000 0000 0000 0000 0000 0000 0000
        0260 | 0000 0000 0000 0000 0000 0000 0000 0000
        0268 | 0000 0000 0000 0000 0000 0000 0000 0000
        0270 | 0000 0000 0000 0000 0000 0000 0000 0000
        0278 | 0000 0000 0000 0000 0000 0000 0000 0000
        0280 | 0000 0000 0000 0000 0000 0000 0000 0000
        0288 | 0000 0000 0000 0000 0000 0000 0000 0000
        0290 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0300
        0298 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0308 
        02a0 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0310 
        02a8 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0318 
        02b0 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0320 
        02b8 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0328 
        02c0 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0330 
        02c8 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0338 
        02d0 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0340 
        02d8 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0348 
        02e0 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0350 
        02e8 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0358 
        02f0 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0360 
        02f8 | 0000 0000 0000 0000 0000 0000 0000 0000 | 0368 
             | 0001 0000 0001 0000 0000 0000 0001 0000 | 0370 
                                                        ...
      This little maneuver reduces our table by 112 entries 
      (14 rows x 8), and that's just for these two blocks.

5. Taking the idea a bit further, we have no constraints on
   where in the table our *zero-filled* block should be.
   
   So we find a pair of blocks that have the largest 
   *zero-filled* overlap and we place our zero-filled
   block between them.
   
   Once the blocks are in this new order, we squish them
   This brings the total number of entries down to **3866**.

6. Finally, the index and the table are merged into a single array.
    
   The `casemap_lower[]` comprises two distinct parts.
   It starts with 256 index entries, followed by 3866 
   deltas, to the grand total of **4122**.
    
   Going back to the `towlower()` code -
    
        ...
        return ch + casemap_lower[casemap_lower[ch >> 8] + (ch & 0xff)];
    
   `casemap_lower[ch >> 8]` is the index lookup which points at the block's 
   start in the data section, followed by the retrieval of ch's delta value 
   within the block at the offset of `ch & 0xff`.
 
Marvelous, isn't it?

If you know the origins of this technique (or its author!), let me know
and I'll add a proper reference.
   
# Table compression, revised

But wait! We can take it a bit further.

## Smaller block sizes
 
Instead of splitting the original 65536 set into 256 x 256 blocks we 
can try different block sizes.

Using larger block sizes will reduce the size of the index, but it
will also cause greater %-age of blocks to contain at least one 
non-zero entry.

The potential for block overlap (as per #4 above) will also increase
for any given pair of blocks, but the number of pairs themselves will 
be smaller.

    Block size  |  Used blocks  |  Data size  |  Overlap  |  Table size
                                                                       
       1024     x       10      =    10240    -   1947    =    8293    
        512     x       13      =     6656    -   1311    =    5345    
        256     x       18      =     4608    -    742    =    3866       <- the original split
        128     x       28      =     3584    -    533    =    3051    
         64     x       44      =     2816    -    582    =    2234    
         32     x       55      =     1760    -    337    =    1423    
         16     x       66      =     1056    -    189    =     867    

The last columns is the number of items in the table. But let's not 
forget the index:

    Block size  |  Index size  |  Table size  |  Total size
                     
       1024             64           8293           8357
        512            128           5345           5473
        256            256           3866           4122       <-  the original
        128            512           3051           3563
         64           1024           2234           3258       <-  the smallest
         32           2048           1423           3471
         16           4096            867           4963
 
 That is, reducing the block size to 64 bytes we can compress the
 table down to 3258 items (**6516 bytes**). This is 21% reduction
 compared to the original of 4122 (or 8244 bytes).
 
 The conversion function itself will look like so:
 
     ...
     return ch + casemap_lower[casemap_lower[ch >> 6] + (ch & 0x3f)];

## Better block sequence

In addition to finding the best spot for the zero-filled
block, we can try and shuffle **all** blocks around and
search for the most squishable combination.

I suspect that there is a better than O(n!) way to do it,
but being the lazy optimizators that we are, we will just
resort to randomized shuffling and an overnight test run.

Here are the results:

    Block size  |     Overlap     |    Total size

        256        742  ->  1196     4122  ->  3668
        128        533  ->   870     3563  ->  3226       <-  the smallest
         64        582  ->   596     3258  ->  3244       
         32        337  ->   339     3471  ->  3469
         16        189  ->   189     4963  ->  4963       <-  no change

That is, block reshuffling can reduce the original Wine
table from **4122** to **3668** items.

Combined with a smaller block size the reshuffling can
produce a **3244** item table.

## Separate index

The table size can be reduced a bit more by noticing that we have 
fewer than 256 unique index entries. So the index can be built
as `uint8_t[]`  instead of `uint16_t[]`.

However this requires using a secondary index as such:

    // for the block size of 64
    
    uint8_t   index[1024]  = { ... };
    uint16_t  offsets[44]  = { ... };
    uint16_t  deltas[2200] = { ... };
    uint16_t  offset = offsets[ index[ ch >> 6 ] ];
    
    ch = deltas[offset + (ch & 0x3f) ];
    
When applied, this change yields the following total byte counts:

    Block size  |  Index[]  |   Offsets[]   |   Deltas[]   |  Total bytes
				     		    	       
       256           256    +    18 x 2     +   3412 x 2   =     7116
       128           512    +    28 x 2     +   2714 x 2   =     5996
        64          1024    +    44 x 2     +   2220 x 2   =     5552
        32          2048    +    55 x 2     +   1421 x 2   =     5000       <-  chicken dinner
        16          4096    +    66 x 2     +    867 x 2   =     5962

## In other words

If we are willing to add an extra memory reference to every case
conversion, we can shrink the original Wine table of 
**8244 bytes** down to **5000 bytes**, a reduction of ~ **40%**.

If we prefer to stick to the original lookup code, we can still
reduce the table size to **6452 bytes**, a reduction of ~ **22%**.

Either way this is just a cherry on top of an already excellent
compression technique.

# Conclusion

Very fast case conversion of (the vast majority of) Unicode characters
can be implemented in a handful of CPU cycles and a precomputed lookup
table of between 5KB to 8KB in size.

The 8KB version, courtesy of Wine:
* [unicode.h](https://github.com/wine-mirror/wine/blob/e909986e6ea5ecd49b2b847f321ad89b2ae4f6f1/include/wine/unicode.h#L93)
* [casemap.c](https://github.com/wine-mirror/wine/blob/e909986e6ea5ecd49b2b847f321ad89b2ae4f6f1/libs/port/casemap.c)

The 5KB version, with better table compression:
* x
* z
