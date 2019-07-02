Disk: Victor 9000
=================

The Victor 9000 / Sirius One was a rather strange old 8086-based machine
which used a disk format very reminiscent of the Commodore format; not a
coincidence, as Chuck Peddle designed them both. They're 80-track, 512-byte
sector GCR disks, with a variable-speed drive and a varying number of sectors
per track --- from 19 to 12. Disks can be double-sided, meaning that they can
store 1224kB per disk, which was almost unheard of back then.

FluxEngine reads these.

Reading discs
-------------

Just do:

```
fluxengine read victor9k
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

