Disk: Macintosh
===============

Macintosh disks come in two varieties: the newer 1440kB ones, which are
perfectly ordinary PC disks you should use `fluxengine read ibm` to read, and
the older 800kB disks (and 400kB for the single sides ones). They have 80
tracks and up to 12 sectors per track.

They are also completely insane.

It's not just the weird, custom GCR encoding. It's not just the utterly
bizarre additional encoding/checksum built on top of that where [every byte
is mutated according to the previous bytes in the
sector](https://www.bigmessowires.com/2011/10/02/crazy-disk-encoding-schemes/).
It's not just the odd way in which disks think they have four sides, two on
one side and two on the other, so that the track byte stores only the bottom
6 bits of the track number. It's not just the way that Macintosh sectors are
524 bytes long. No, it's the way the Macintosh drive changes speed depending
on which track it's looking at, so that each track contains a different
amount of data.

The reason for this is actually quite sensible: the tracks towards the centre
of the disk are obviously moving more slowly, so you can't pack the bits in
quite as closely (due to limitations in the magnetic media). You can use a
higher bitrate at the edge of the disk than in the middle. Many platforms,
for example the Commodore 64 1541 drive, changed bitrate this way.

But Macintosh disks used a constant bitrate and changed the speed that the
disk spun instead to achieve the same effect...

_Anyway_: FluxEngine will read them fine on conventional drives.
Because it's clever.

**Big note.** Apparently --- and I'm still getting to the bottom of this ---
some drives work and some don't. My drives produce about 90% good reads of
known good disks. One rumour I've heard is that drives sometimes include
filters which damage the signals at very particular intervals which Mac disks
use, but frankly this seems unlikely; it could be a software issue at my end
and I'm investigating. If you have any insight, please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).

Reading discs
-------------

Just do:

```
fluxengine read mac
```

You should end up with an `mac.img` which is 1001888 bytes long (for a normal
DD disk). If you want the single-sided variety, use `-s :s=0`.

**Big warning!** The image may not work in an emulator. Mac disk images are
complicated due to the way the tracks are different sizes and the odd sector
size. FluxEngine chooses to store them in a simple 524 x 12 x 2 x 80 layout,
with holes where missing sectors should be. This was easiest. If anyone can
suggest a better way, please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).

The 12 bytes of metadata _follow_ the 512 bytes of user payload in the sector
image. If you don't want it, specify a geometry in the output file with a
512-byte sectore size like `-o mac.img:c=80:h=1:s=12:b=512`.

Useful references
-----------------

  - [MAME's ap_dsk35.cpp file](https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib/formats/ap_dsk35.cpp),
    without which I'd never have managed to do this

  - [Crazy Disk Encoding
    Schemes](https://www.bigmessowires.com/2011/10/02/crazy-disk-encoding-schemes/), which made
    me realise just how nuts the format is

  - [Les Disquettes et le drive Disk II](http://www.hackzapple.com/DISKII/DISKIITECH.HTM), an
    epicly detailed writeup of the Apple II disk format (which is closely related)
