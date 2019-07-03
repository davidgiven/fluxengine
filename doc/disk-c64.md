Disk: Commodore 64
==================

Commodore 64 disks come in two varieties: GCR, which are the overwhelming
majority; and MFM, only used on the 1571 and 1581. The latter were (as far as
I can tell) standard IBM PC format disks, so use `fluxengine read ibm` to
read them (and then [let me know if it
worked](https://github.com/davidgiven/fluxengine/issues/new).

The GCR disks are much more interesting. They could store 170kB on a
single-sided disk (although later drives were double-sided), using a proprietary
encoding and record scheme; like [Apple Macintosh disks](macintosh.md) they
stored varying numbers of sectors per track to make the most of the physical
disk area, although unlike them they did it by changing the bitrate rather than
adjusting the motor speed.

The drives were also intelligent and ran DOS on a CPU inside them. The
computer itself knew nothing about file systems. You could even upload
programs onto the drive and run them there, allowing all sorts of custom disk
formats, although this was mostly used to compensate for the [cripplingly
slow connection to the
computer](https://ilesj.wordpress.com/2014/05/14/1541-why-so-complicated/) of
300 bytes per second (!). (The drive itself could transfer data reasonably
quickly.)

A standard 1541 disk has 35 tracks of 17 to 20 sectors, each 256 bytes long.

Reading discs
-------------

Just do:

```
fluxengine read c64
```

You should end up with an `c64.img` which is 187136 bytes long (for a normal
1541 disk).

**Big warning!** The image may not work in an emulator. Commodore 64 disk images are
complicated due to the way the tracks are different sizes and the odd sector
size. FluxEngine chooses to store them in a simple 256 x 20 x 35 layout,
with holes where missing sectors should be. This was easiest. If anyone can
suggest a better way, please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).

Useful references
-----------------

  - [Ruud's Commodore Site: 1541](http://www.baltissen.org/newhtm/1541c.htm):
    documentation on the 1541 disk format.

