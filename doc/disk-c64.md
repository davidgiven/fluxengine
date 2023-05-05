Disk: Commodore 64
==================

Commodore 64 disks come in two varieties: GCR, which are the overwhelming
majority; and MFM, only used on the 1571 and 1581. The latter were (as far as I
can tell) standard IBM PC format disks with a slightly odd sector count.

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

A standard 1541 disk has 35 tracks of 17 to 21 sectors, each 256 bytes long
(sometimes 40 tracks).

A standard 1581 disk has 80 tracks and two sides, each with 10 sectors, 512
bytes long.

Reading 1541 disks
------------------

Just do:

```
fluxengine read commodore -o commodore.d64
```

You should end up with an `commodore.d64` file which is 174848 bytes long.
You can load this straight into a Commodore 64 emulator such as
[VICE](http://vice-emu.sourceforge.net/).

If you have a 40-track disk, add `--196`.

**Big warning!** Commodore 64 disk images are complicated due to the way the
tracks are different sizes and the odd sector size, so you need the special D64
or LDBS output formats to represent them sensibly.

Writing 1541 disks
------------------

Just do:
```
fluxengine write commodore -i file.d64
```

If you have a 40-track disk, add `--196`.

Note that only standard Commodore 64 BAM file systems can be written this way,
as the disk ID in the BAM has to be copied to every sector on the disk.

Reading and writing 1581 disks
------------------------------

1581 disks are just another version of the standard IBM scheme.

Just do:

```
fluxengine read commodore1581 -o commodore1581.d81
```

or:

```
fluxengine write commodore1581 -i commodore1581.img
```

Reading and writing CMD FD2000 disks
------------------------------------

Yet again, these are another IBM scheme variant.

Just do:

```
fluxengine read cmd_fd2000 -o cmd_fd2000.d81
```

or:

```
fluxengine write cmd_fd2000 -i cmd_fd2000.img
```


Useful references
-----------------

  - [Ruud's Commodore Site: 1541](http://www.baltissen.org/newhtm/1541c.htm):
    documentation on the 1541 disk format.

