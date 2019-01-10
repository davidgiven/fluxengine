Brother word processor disks
============================

Brother word processor disks are weird, using custom tooling and chipsets.
They are completely not PC compatible in every possible way other than the
size.

Different word processors use different disk formats --- the only one
supported by FluxEngine is the 240kB 3.5" format.

Reading discs
-------------

Just do:

```
.obj/fe-readbrother --revolutions=2
```

You should end up with a `brother.img` which is 239616 bytes long. (If you
don't specify `--revolutions`, you may end up with a partial image.)

Low level format
----------------

The drive is a single-sided 3.5" drive spinning at not 300 rpm (I don't
know the precise speed yet but FluxEngine doesn't care). The disks have 78
tracks. The Brother drive alignment is kinda variable so on my machine
logical track 0 is at physical track 3, but I've also seek logical track 0
at physical track 1.

Each track has 12 256-byte sectors. The drive ignores the index hole so they're
lined up all anyhow. As FluxEngine can only read from index to index, it
actually reads two complete revolutions and reassembles the sectors from that.

The underlying encoding is exceptionally weird; they use two different kinds of
GCR, one kind for the sector header records and a completely different one for
the data itself. It also has a completely bizarre CRC variant which a genius on
StackOverflow reverse engineered for me. However, odd though it may be, it does
seem pretty robust.

See the source code for the GCR tables and CRC routine.

Sectors are about 16.7ms apart on the disk (at 300 rpm). The header and
data records are 0.694ms apart. (All measured from the beginning of the
record.) The sector order is 05a3816b4927, which gives a sector skew of 5.

High level format
-----------------

Once decoded, you end up with a 234kB file system image. Luckily, this turns
out to be a completely normal FAT file system with a media type of 0x58 --- did
you know that FAT supports 256 byte sectors? I didn't --- of the MSX-DOS
variety. There's a faint possibility that the word processor is based on
MSX-DOS, but I haven't reverse engineered it to find out.

Standard Linux mtools will access the filesystem image and allow you to move
files in and out. However, you'll need to change the media type bytes at
offsets 0x015 and 0x100 from 0x58 to 0xf0 before mtools will touch it. Once
done, this will work:

```
mdir -i brother.img
mcopy -i brother.img ::brother.doc linux.doc
```

Converting the equally proprietary file format to something readable is,
unfortunately, out of scope for FluxEngine.

