Victor 9000 disks
=================

**Warning.** This is experimental; I haven't found a clean disk to read yet.
The fragmented disk images which I have found ([from
vintagecomputer.ca](http://vintagecomputer.ca/files/Victor%209000/)) get
about 57% good sectors. It could just be that the disks are bad, but there
could also be something wrong with my decode logic. If you have any Victor
disks and want to give this a try for real, [please get in
touch](https://github.com/davidgiven/fluxengine/issues/new).

The Victor 9000 / Sirius One was a rather strange old 8086-based machine
which used a disk format very reminiscent of the Commodore format; not a
coincidence, as Chuck Peddle designed them both. They're 80-track, 512-byte
sector GCR disks, with a variable-speed drive and a varying number of sectors
per track --- from 19 to 12. Reports are that they're double-sided but I've
never seen a double-sided disk image.

FluxEngine reads them (subject to the warning above).

Reading discs
-------------

Just do:

```
.obj/fe-readvictor9k
```

You should end up with an `victor9k.img` which is 774656 bytes long.
if you want the double-sided variety, use `-s :s=0-1`.

**Big warning!** The image may not work in an emulator. Victor disk images are
complicated due to the way the tracks are different sizes and the odd sector
size. FluxEngine chooses to store them in a simple 512 x 19 x 1 x 80 layout,
with holes where missing sectors should be. This was easiest. If anyone can
suggest a better way, please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).


Useful references
-----------------

  - [The Victor 9000 technical reference manual](http://bitsavers.org/pdf/victor/victor9000/Victor9000TechRef_Jun82.pdf)

  - [DiskFerret's Victor 9000 format guide](https://discferret.com/wiki/Victor_9000_format)

