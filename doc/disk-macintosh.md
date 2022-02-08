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

Reading discs
-------------

Just do:

```
fluxengine read <format> -o mac.dsk
```

...where `<format>` can be `mac400` or `mac800`.

You should end up with a `mac.dsk` file containing a raw sector image
(equivalent to `.img`).

**Big warning!** Mac disk images are complicated due to the way the tracks are
different sizes and the odd sector size. What you get above is a triangular
disk image, which contains all the 512-byte user data areas concatenated
together in filesystem order. It does not contain the twelve bytes of metadata.
If you want these as well, specify that you want 524 byte sectors with
`--output.image.img.trackdata.sector_size=512`. The metadata will follow the
512 bytes of user data.

FluxEngine also supports DiskCopy 4.2 disk images, which may be a better option
if you're going to be using the image on a real Macintosh (or with other
tooling which supports it). To get one of these, use a filename like
`mac.diskcopy`. This will contain both user data and metadata.

Writing discs
-------------

Just do:

```
fluxengine write <format> -i mac.dsk
```

...where `<format>` can be `mac400` or `mac800`.

It'll read the image file and write it out.

The same warning as above applies --- you can use normal `.dsk` files but it's
problematic. Consider using DiskCopy 4.2 files instead.

Useful references
-----------------

  - [MAME's ap_dsk35.cpp file](https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib/formats/ap_dsk35.cpp),
    without which I'd never have managed to do this

  - [Crazy Disk Encoding
    Schemes](https://www.bigmessowires.com/2011/10/02/crazy-disk-encoding-schemes/), which made
    me realise just how nuts the format is

  - [Les Disquettes et le drive Disk II](http://www.hackzapple.com/DISKII/DISKIITECH.HTM), an
    epicly detailed writeup of the Apple II disk format (which is closely related)

  - [The DiskCopy 4.2
	format](https://www.discferret.com/wiki/Apple_DiskCopy_4.2), described on
	the DiskFerret website.
