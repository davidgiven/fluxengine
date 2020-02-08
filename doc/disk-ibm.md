Disk: Generic IBM
=================

IBM scheme disks are _the_ most common disk format, ever. They're used by a
huge variety of different systems, and they come in a huge variety of different
forms, but they're all fundamentally the same: either FM or MFM, either single
or double sided, with distinct sector header and data records and no sector
metadata. Systems which use IBM scheme disks include but are not limited to:

  - IBM PCs (naturally)
  - Atari ST
  - late era Apple machines
  - Acorn machines
  - the TRS-80
  - late era Commodore machines (the 1571 and so on)
  - most CP/M machines
  - etc

FluxEngine supports reading these. However, some variants are more peculiar
than others, and as a result there are specific decoders which set the defaults
correctly for certain formats (for example: on PC disks the sector numbers
start from 1, but on [Acorn](disk-acorndfs.md) disks they start from 0). The
IBM decoder described here is the generic one, and is suited for 'conventional'
PC disks. While you can read all the variant formats with it if you use the
right set of arguments, it's easier to use the specific decoder.

The generic decoder is mostly self-configuring, and will detect the format of
your disk for you.


Reading disks
-------------

Just do:

    fluxengine read ibm

...and you'll end up with an `ibm.img` file. This should work on most PC disks
(including FM 360kB disks, 3.5" 1440kB disks, 5.25" 1200kB disks, etc.) The size
of the disk image will vary depending on the format.

Configuration options you'll want include:

  - `--sector-id-base`: specifies the ID of the first sector; this defaults
    to 1. Some formats (like the Acorn ones) start at 0. This can't be
	autodetected because FluxEngine can't distinguish between a disk which
	starts at sector 1 and a disk which starts at sector 0 but all the sector
	0s are missing.

  - `--ignore-side-byte`: each sector header describes the location of the
	sector: sector ID, track and side. Some formats use the wrong side ID, so
	the sectors on side 1 are labelled as belonging to side 0. This causes
	FluxEngine to see duplicate sectors (as it can't distinguish between the
	two sides). This option tells FluxEngine to ignore the side byte completely
	and use the physical side instead.


Reading mixed-format disks
--------------------------

Some disks, usually those belonging to early CP/M machines, have more than one
format on the disk at once. Typically, the first few tracks will be low-density
FM encoded and will be read by the machine's ROM; those tracks contain new
floppy drive handling code capable of coping with MFM data, and so the rest of
the disk will use that, allowing them to store more data.

FluxEngine copes with these fine, but the disk images are a bit weird. If track
0 is FM and contains five sectors, but track 1 is MFM with nine sectors (MFM is
more efficient and the sectors are physically smaller, allowing you to get more
on), then the resulting image will have nine sectors per track... but track 0
will only contain data in the first five.

This is typically what you want as it makes locating the sectors in the image
easier, but some emulators may require a different format. Please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new) if you have
specific requirements (nothing's come up yet). Alternatively, you can tell
FluxEngine to write a [`.ldbs`
file](http://www.seasip.info/Unix/LibDsk/ldbs.html) and then use
[libdsk](http://www.seasip.info/Unix/LibDsk/) to convert it to something
useful.

