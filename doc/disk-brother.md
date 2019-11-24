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
real track is, but normal PC drives aren't capable of doing this.
Particularly with the 120kB disks, you might want to fiddle with the start
track (e.g. `:t=0-79x2`) to get a clean read. Keep an eye on the bad sector
map that's dumped at the end of a read.

Using FluxEngine to *write* disks isn't a problem, so the
simplest solution is to use FluxEngine to create a new disk, with the tracks
aligned properly, and then use a word processor to copy the files you want
onto it. The new disk can then be read and you can extract the files.
Obviously this sucks if you don't actually have a word processor, but I can't
do anything about that.

If you find one of these misaligned disks then *please* [get in
touch](https://github.com/davidgiven/fluxengine/issues/new); I want to
investigate.

Reading discs
-------------

Just do:

```
fluxengine read brother
```

You should end up with a `brother.img` which is 239616 bytes long.

Writing discs
-------------

Just do:

```
fluxengine write brother
```

...and it'll write a `brother.img` file which is 239616 bytes long to the
disk. (Use `-i` to specify a different input filename.)

Low level format
----------------

The drive is a single-sided 3.5" drive spinning at not 300 rpm (I don't know
the precise speed yet but FluxEngine doesn't care). The 240kB disks have 78
tracks and the 120kB disks have 39.

The Brother drive alignment is kinda variable; when you put the disk in the
drive it seeks all the way to physical track 0 and then starts searching for
something which looks like data. My machine likes to put logical track 0 on
physical track 3. FluxEngine puts logical track 0 on physical track 0 for
simplicity, which works fine (at least on my machine). If this doesn't work
for you, [get in touch](https://github.com/davidgiven/fluxengine/issues/new);
there are potential workarounds.

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
.obj/brother120tool image.img
```

To extract a file, do:

```
.obj/brother120tool image.img filename
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
change it back to 0x58 before writing an image to disk.

The file format is not WP-1, and currently remains completely unknown,
although it's probably related. If anyone knows anything about this, please
[get in touch](https://github.com/davidgiven/fluxengine/issues/new).
