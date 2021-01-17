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

  - `--ibm-sector-id-base=N`: specifies the ID of the first sector; this defaults
	to 1. Some formats (like the Acorn ones) start at 0. This can't be
	autodetected because FluxEngine can't distinguish between a disk which
	starts at sector 1 and a disk which starts at sector 0 but all the sector
	0s are missing.

  - `--ibm-ignore-side-byte=true|false`: each sector header describes the location of the
	sector: sector ID, track and side. Some formats use the wrong side ID, so
	the sectors on side 1 are labelled as belonging to side 0. This causes
	FluxEngine to see duplicate sectors (as it can't distinguish between the
	two sides). This option tells FluxEngine to ignore the side byte completely
	and use the physical side instead.

  - `--ibm-required-sectors=range`: if you know how many sectors to expect per
	track, you can improve reads by telling FluxEngine what to expect here. If
	a track is read and a sector on this list is _not_ present, then FluxEngine
	assumes the read failed and will retry. This avoids the situation where
	FluxEngine can't tell the difference between a sector missing because it's
	bad or a sector missing because it was never written in the first place. If
	sectors are seen outside the range here, it will still be read. You can use
	the same syntax as for track specifiers: e.g. `0-9`, `0,1,2,3`, etc.


Writing disks
-------------

FluxEngine can also write IBM scheme disks. Unfortunately the format is
incredibly flexible and you need to specify every single parameter, which
makes things slightly awkward.

The syntax is:

    fluxengine write ibm -i input.img <options>

The format of `input.img` will vary depending on the kind of disk you're
writing, which is configured by the options. There are some presets, which
you will almost certainly want to use if possible:

  - `--ibm-preset-720`: a standard 720kB DS DD 3.5" disk, with 80 cylinders,
  2 sides, and 9 sectors per track.
  - `--ibm-preset-1440`: a standard 1440kB DS HD 3.5" disk, with 80
  cylinders, 2 sides, and 18 sectors per track.

These options simply preset the following, lower-level options. Note that
options are processed left to right, so it's possible to use a preset and
then change some settings. To see the values for a preset, simply append
`--help`.

  - `--ibm-track-length-ms=N`: one disk rotation, in milliseconds. This is used
  to determine whether all the data will fit on a track or not. `fluxengine
  rpm` will tell you this; it'll be 200 for a normal 3.5" drive and 166 for a
  normal 5.25" drive.
  - `--ibm-sector-size=N`: the size of a sector, in bytes. Must be a power of
  two.
  - `--ibm-emit-iam=true|false`: whether to emit the IAM record at the top of
  the track. The standard format requires it, but it's ignored by absolutely
  everyone and you can fit a bit more data on the disk without it.
  - `--ibm-start-sector-id=N`: the sector ID of the first sector. Normally 1,
  except for non-standard formats like Acorn's, which use 0.
  - `--ibm-use-fm=true|false`: uses FM rather than MFM.
  - `--ibm-idam-byte=N`: the sixteen-bit raw bit pattern used for the IDAM ID
  byte. Big-endian, clock bit first.
  - `--ibm-dam-byte-N`: the sixteen-bit raw bit pattern used for the DAM ID
  byte. Big-endian, clock bit first.
  - `--ibm-gap0-bytes=N`: the size of gap 0 in bytes (between the start of
  the track and the IAM record).
  - `--ibm-gap1-bytes=N`: the size of gap 1 in bytes (between the IAM record
  and the first sector record).
  - `--ibm-gap2-bytes=N`: the size of gap 2 in bytes (between each sector
  record and the data record).
  - `--ibm-gap3-bytes=N`: the size of gap 3 in bytes (between the data record
  and the next sector record).
  - `--ibm-sector-skew=0123...`: a string representing the order in which to
  write sectors: each character represents on sector, with `0` being the
  first (always, regardless of `--ibm-start-sector-id` above). Sectors 10 and
  above are represented as letters from `A` up.

Mixed-format disks
------------------

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
easier, but emulators will typically require a different format. Please [get
in touch](https://github.com/davidgiven/fluxengine/issues/new) if you have
specific requirements (nothing's come up yet). Alternatively, you can tell
FluxEngine to write a [`.ldbs`
file](http://www.seasip.info/Unix/LibDsk/ldbs.html) and then use
[libdsk](http://www.seasip.info/Unix/LibDsk/) to convert it to something
useful.

One easy option when reading these is to simply read the two sections of the
disk into two different image files.

FluxEngine can write these too, but in two different passes with different
options. It's possible to assemble a flux file by judicious use of `-D
something.flux --merge`, which can then be written in a single pass with
`fluxengine writeflux`, but it's usually not worth the bother: just write the
boot tracks, then write the data tracks, possibly with a script for automation.
