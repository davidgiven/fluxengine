Disk: Brother word processor
============================

Brother word processor disks are weird, using custom tooling and chipsets.
They are completely not PC compatible in every possible way other than the
size.

Different word processors use different disk formats --- the only ones
supported by FluxEngine are the 120kB and 240kB 3.5" formats. The default
options are for the 240kB format. For the 120kB format, which is 40 track, do
`fluxengine read brother -s :t=1-79x2`.

Apparently about 20% of Brother word processors have alignment issues which
means that the disks can't be read by FluxEngine (because the tracks on the
disk don't line up with the position of the head in a PC drive). The word
processors themselves solved this by microstepping until they found where the
real track is, but normal PC drives aren't capable of doing this.  Particularly
with the 120kB disks, you might want to fiddle with the start track (e.g.
`:t=0-79x2`) to get a clean read. Keep an eye on the bad sector map that's
dumped at the end of a read. My word processor likes to put logical track 0 on
physical track 3, which means that logical track 77 is on physical track 80;
luckily my PC drive can access track 80.

Using FluxEngine to *write* disks isn't a problem, so the
simplest solution is to use FluxEngine to create a new disk, with the tracks
aligned properly, and then use a word processor to copy the files you want
onto it. The new disk can then be read and you can extract the files.
Obviously this sucks if you don't actually have a word processor, but I can't
do anything about that.

If you find one of these misaligned disks then *please* [get in
touch](https://github.com/davidgiven/fluxengine/issues/new); I want to
investigate.

Reading disks
-------------

Just do:

```
fluxengine read `<format>`
```

... where `<format>` can be `brother120` or `brother240`. You should end up
with a `brother.img` which is either 119808 or 239616 bytes long.

Writing disks
-------------

Just do:

```
fluxengine write `<format>` -i brother.img
```

...where `<format>` can be `brother120` or `brother240`.

Dealing with misaligned disks
-----------------------------

While FluxEngine can't read misaligned disks directly, Brother word processors
_can_. If you have access to a compatible word processor, there's a fairly
simple workaround to allow you to extract the data:

  1. Format a disk using FluxEngine (by simply writing a blank filesystem image
	 to a disk). This will have the correct alignment to work on a PC drive.

  2. Use a word processor to copy the misaligned disk to the newly formatted
	 disk. The machine will happily adjust itself to both sets of alignments.

  3. Use FluxEngine to read the data off the correctly aligned disk.

I realise this is rather unsatisfactory, as the Brother hardware is becoming
rarer and they cope rather badly with damaged disks, but this is a limitation
of the hardware of normal PC drives. (It _is_ possible to deliberately misalign
a drive to make it match up with a bad disk, but this is for experts only --- I
wouldn't dare.)

Low level format
----------------

The drive is a single-sided 3.5" drive spinning at not 300 rpm (I don't know
the precise speed yet but FluxEngine doesn't care). The 240kB disks have 78
tracks and the 120kB disks have 39.

Each track has 12 256-byte sectors. The drive ignores the index hole so they're
lined up all anyhow. As FluxEngine can only read from index to index, it
actually reads two complete revolutions and reassembles the sectors from that.

The underlying encoding is exceptionally weird; they use two different kinds of
GCR, one kind for the sector header records and a completely different one for
the data itself. It also has a completely bizarre CRC variant which a genius on
StackOverflow reverse engineered for me. However, odd though it may be, it does
seem pretty robust.

See the source code for the GCR tables and CRC routine.

Sectors are about 16.2ms apart on the disk (at 300 rpm). The header and
data records are 0.694ms apart. (All measured from the beginning of the
record.) The sector order is 05a3816b4927, which gives a sector skew of 5.

High level format
-----------------

Once decoded, you end up with a file system image.

### 120kB disks

These disks use a proprietary and very simple file system. It's FAT-like
with an obvious directory and allocation table. I have reversed engineered
a very simple tool for extracting files from it. To show the directory, do:

```
brother120tool image.img
```

To extract a file, do:

```
brother120tool image.img filename
```

Wildcards are supported, so use `'*'` for the filename (remember to quote it)
if you want to extract everything.

The files are usually in the format known as WP-1, which aren't well
supported by modern tools (to nobody's great surprise). Matthias Henckell has
[reverse engineered the file
format](https://mathesoft.eu/brother-wp-1-dokumente/) and has produced a
(Windows-only, but runs in Wine) [tool which will convert these files into
RTF](https://mathesoft.eu/sdm_downloads/wp2rtf/). This will only work on WP-1
files.

To create a disk image (note: this creates a _new_ disk image, overwriting the
previous image), do:

```
brother120tool --create image.img filename1 filename2...
```

Any files whose names begin with an asterisk (`*`) will be marked as hidden. If
the file is named `*boot`, then a boot sector will be created which will load
and run the file at 0x7000 if the machine is started with CODE+Q pressed. So
far this has only been confirmed to work on a WP-1.

Any questions? please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).

### 240kB disks

Conversely, the 240kB disks turns out to be a completely normal Microsoft FAT
file system with a media type of 0x58 --- did you know that FAT supports 256
byte sectors? I didn't --- of the MSX-DOS variety. There's a faint
possibility that the word processor is based on MSX-DOS, but I haven't
reverse engineered it to find out.

Standard Linux mtools will access the filesystem image and allow you to move
files in and out. However, you'll need to change the media type bytes at
offsets 0x015 and 0x100 from 0x58 to 0xf0 before mtools will touch it. The
supplied `brother240tool` will do this. Once done, this will work:

```
mdir -i brother.img
mcopy -i brother.img ::brother.doc linux.doc
```

The word processor checks the media byte, unfortunately, so you'll need to
change it back to 0x58 before writing an image to disk. Just run
`brother240tool` on the image again and it will flip it back.

The file format is not WP-1, and currently remains completely unknown,
although it's probably related. If anyone knows anything about this, please
[get in touch](https://github.com/davidgiven/fluxengine/issues/new).
