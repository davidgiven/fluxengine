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

A standard 1541 disk has 35 tracks of 17 to 21 sectors, each 256 bytes long.

Reading discs
-------------

Just do:

```
fluxengine read c64
```

You should end up with an `c64.d64` file which is 174848 bytes long. You can
load this straight into a Commodore 64 emulator such as
[VICE](http://vice-emu.sourceforge.net/).

**Big warning!** Commodore 64 disk images are
complicated due to the way the tracks are different sizes and the odd sector
size, so you need the special D64 or LDBS output formats to represent them
sensibly. Don't use IMG unless you know what you're doing.

Useful references
-----------------

  - [Ruud's Commodore Site: 1541](http://www.baltissen.org/newhtm/1541c.htm):
    documentation on the 1541 disk format.

