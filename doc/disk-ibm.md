Disk: Generic IBM
=================

IBM scheme disks are _the_ most common disk format, ever. They're used by a
huge variety of different systems, and they come in a huge variety of different
forms, but they're all fundamentally the same: either FM or MFM, either single-
or double-sided, with distinct sector header and data records and no sector
metadata. Systems which use IBM scheme disks include but are not limited to:

  - IBM PCs (naturally)
  - Atari ST
  - late era Apple machines
  - Acorn machines
  - the TRS-80
  - late era Commodore machines (the 1571 and so on)
  - most CP/M machines
  - NEC PC-88 series
  - NEC PC-98 series
  - Sharp X68000
  - Fujitsu FM Towns
  - VAX & PDP-11
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

    fluxengine read `<format>`

...and you'll end up with a `<format>.img` file. This should work on most PC
disks (including FM 360kB disks, 3.5" 1440kB disks, 5.25" 1200kB disks, etc.)
The size of the disk image will vary depending on the format.

The common PC formats are `ibm720` and `ibm1440`, but there are _many_ others,
and there's too many configuration options to usefully list. Use `fluxengine
write` to list all formats, and try `fluxengine write ibm1440 --config` to see
a sample configuration.

Configuration options you'll want include:

  - `--decoder.ibm.sector_id_base=N`: specifies the ID of the first sector;
	this defaults to 1. Some formats (like the Acorn ones) start at 0. This
	can't be autodetected because FluxEngine can't distinguish between a disk
	which starts at sector 1 and a disk which starts at sector 0 but all the
	sector 0s are missing.

  - `--decoder.ibm.ignore_side_byte=true|false`: each sector header describes
	the location of the sector: sector ID, track and side. Some formats use the
	wrong side ID, so the sectors on side 1 are labelled as belonging to side
	0. This causes FluxEngine to see duplicate sectors (as it can't distinguish
	between the two sides). This option tells FluxEngine to ignore the side
	byte completely and use the physical side instead.

  - `--decoder.ibm.required_sectors=range`: if you know how many sectors to
	expect per track, you can improve reads by telling FluxEngine what to
	expect here. If a track is read and a sector on this list is _not_ present,
	then FluxEngine assumes the read failed and will retry. This avoids the
	situation where FluxEngine can't tell the difference between a sector
	missing because it's bad or a sector missing because it was never written
	in the first place. If sectors are seen outside the range here, it will
	still be read. You can use the same syntax as for track specifiers: e.g.
	`0-9`, `0,1,2,3`, etc.


Writing disks
-------------

FluxEngine can also write IBM scheme disks. Unfortunately the format is
incredibly flexible and you need to specify every single parameter, which
makes things slightly awkward. Preconfigured profiles are available.

The syntax is:

    fluxengine write <format> -i input.img <options>

The common PC formats are `ibm720` and `ibm1440`, but there are _many_ others,
and there's too many configuration options to usefully list. Use `fluxengine
write` to list all formats, and try `fluxengine write ibm1440 --config` to see
a sample configuration.

Some image formats, such as DIM, specify the image format, For these you can
specify the `ibm` format and FluxEngine will automatically determine the
correct format to use.

Mixed-format disks
------------------

Some disks, such as those belonging to early CP/M machines, or N88-Basic disks
(for PC-88 and PC-98), have more than one format on the disk at once. Typically,
the first few tracks will be low-density FM encoded and will be read by the
machine's ROM; those tracks contain new floppy drive handling code capable of
coping with MFM data, and so the rest of the disk will use that, allowing them
to store more data.

FluxEngine can read these fine, but it tends to get a bit confused when it sees
tracks with differing numbers of sectors --- if track 0 has 32 sectors but
track 1 has 16, it will assume that sectors 16..31 are missing on track 1 and
size the image file accordingly. This can be worked around by specifying the
size of each track; see the `eco1` read profile for an example.

N88-Basic format floppies can be written by either specifying the `n88basic`
format, or by using D88 or NFD format images which include explicit sector
layout information.

Writing other formats can be made to work too, by creating a custom format
specifier, using the `n88basic` format as an example.
Please [get in touch](https://github.com/davidgiven/fluxengine/issues/new) if
you have specific requirements.

360rpm 3.5" disks
-----------------

Japanese PCs (NEC PC-98, Sharp X68000, Fujitsu FM Towns) spin their floppy
drives at 360rpm rather than the more typical 300rpm. This was done in order
to be fully backwards compatible with 5.25" disks, while using the exact
same floppy controller. Later models of the PC-9821, as well as most USB floppy
drives, feature "tri-mode" support which in addition to normal 300rpm modes,
can change their speed to read and write 360rpm DD and HD disks.

Neither the FluxEngine or Greaseweazle hardware can currently command a
tri-mode drive to spin at 360rpm, however an older 360rpm-only drive will work
to read these formats.

Alternately, the FluxEngine software can rescale the flux pulses to enable
reading and writing these formats with a plain 300rpm drive. To do this,
specify the following two additional options:

    --flux_source.rescale=1.2 --flux_sink.rescale=1.2
