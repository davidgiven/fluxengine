Using a FluxEngine
==================

So you've [built the hardware](building.md), programmed and tested it! What
now?

## The programs

I'm sorry to say that the client program is very badly documented --- it's
moving too quickly for the documentation to keep up. It does respond to
`--help` or `help` depending on context. There are some common properties,
described below.

### Core concepts

FluxEngine fundamentally takes file system images and puts them on disk; or
reads the disk and produces a file system image.

A file system image typically has the extension `.img`. It contains a
sector-by-sector record of the _decoded_ data on the disk. For example, on a
disk with 512 byte sectors, one sector will occupy 512 bytes. These are
typically what you want in everyday life.

FluxEngine can also record the raw magnetic data on the disk into a file, which
we call a _flux file_. This contains all the low-level data which the drive
produced as the disk rotated. These are continuous streams of samples from the
disk and are completely useless in day-to-day life. FluxEngine uses its own
format for this, `.flux`, although it's capable of limited interchange with
Kryoflux, Supercard Pro and Catweasel files. A flux file will typically contain
from 80 to 150 kilobytes of data per track.

In general, FluxEngine can use either a real disk or a flux file
interchangeably: you can specify either at (very nearly) any time. A very
common workflow is to read a disk to a flux file, and then reread from the flux
file while changing the decoder options, to save disk wear. It's also much faster.

### Connecting it up

To use, simply plug your FluxEngine into your computer and run the client. If a
single device is plugged in, it will be automatically detected and used.

If _more_ than one device is plugged in, you need to specify which one to use
with the `--device` parameter, which takes the device serial number as a
parameter.  You can find out the serial numbers by running the command without
the `--device` parameter, and if more than one device is attached they will be
listed. The serial number is also shown whenever a connection is made.

You _can_ work with more than one FluxEngine at the same time, using different
invocations of the client; but be careful of USB bandwidth. If the devices are
connected via the same hub, the bandwidth will be shared.

### Source and destination specifiers

When reading from or writing _flux_ (either from or to a real disk, or a flux
file), use the `--source` (`-s`) and `--dest` (`-d`) options to tell FluxEngine
which bits of the disk you want to access. These use a common syntax:

```
fluxengine read ibm -s fakedisk.flux:t=0-79:s=0
```

  - To access a real disk, leave out the filename (so `:t=0-79:s=0`).

  - To access only some tracks, use the `t=` modifier. To access only some
    sides, use the `s=` modifier.

  - Inside a modifier, you can use a comma separated list of ranges. So
    `:t=0-3` and `:t=0,1,2,3` are equivalent.

  - When specifying a range, you can also specify the step. For example,
    `:t=0-79x2` would be used when accessing a 40-track disk with double
    stepping.

  - To read from drive 1 instead of drive 0, use `:d=1`.

  - To read from a set of KryoFlux stream files, specify the path to the
    directory containing the files _with a trailing slash_; so
    `some/files/:t=0-10`. There must be a files for a single disk only
    in the directory.

Source and destination specifiers work entirely in *physical units*.
FluxEngine is intended to be connected to an 80 (or 82) track double sided
drive, and these are the units used. If the format you're trying to access
lays out its tracks differently, then you'll need a specifier which tells
FluxEngine how to find those tracks. See the 40-track disk example above.

If you _don't_ specify a modifier, you'll get the default, which should be
sensible for the command you're using.

**Important note:** FluxEngine _always_ uses zero-based units (even if the
disk format says otherwise).

### Input and output specifiers

When reading or writing _file system images_, use the `--input` (`-i`) and
`--output` (`-o`) options to specify the file and file format. These use a very
similar syntax to the source and destination specifiers (because they're based
on the same microformat library!) but with different specifiers. Also, the
exact format varies according to the extension:

  - `.img` or `.adf`: raw sector images in CHS order. Append
    `:c=80:h=2:s=9:b=512` to set the geometry; that specifies 80 cylinders, 2
    heads, 9 sectors, 512 bytes per sector. For output files (`--output`) the
    geometry will be autodetected if left unspecified. For input files you
    normally have to specify it.

  - `.ldbs`: John Elliott's [LDBS disk image
    format](http://www.seasip.info/Unix/LibDsk/ldbs.html), which is
    consumable by the [libdsk](http://www.seasip.info/Unix/LibDsk/) suite of
    tools. This allows things like variable numbers of sectors per track
    (e.g. Macintosh or Commodore 64) and also provides information about
    whether sectors were read correctly. You can use libdsk to convert this
    to other formats, using a command like this:

    ```
    $ dsktrans out.ldbs -otype tele out.td0
    ```

    ...to convert to TeleDisk format. (Note you have to use dsktrans rather
    than dskconv due to a minor bug in the geometry hadnling.)

    FluxEngine's LDBS support is currently limited to write only, and
    it doesn't store a lot of the more esoteric LDBS features like format
    types, timings, and data rates.

  - `.d64`: the venerable Commodore 64 disk image format as used by the 1540,
    1541, etc. This is a special-purpose format due to the weird layout of
    1540 disks and while you can use this for non-Commodore disks the result
    will be gibberish. Use this to image Commodore 64 disks and load the
    result into an emulator.

    FluxEngine's D64 support is currently limited to write only. It will work
    with up to 40 logical tracks.

  - `.diskcopy`: a Macintosh DiskCopy 4.2 file. This is a special-purpose
	format due to the weird layout of Mac GCR disks, but it can also support
	720kB and 1440kB IBM disks (although there's no real benefit).

### High density disks

High density disks use a different magnetic medium to low and double density
disks, and have different magnetic properties. 3.5" drives can usually
autodetect what kind of medium is inserted into the drive based on the hole
in the disk casing, but 5.25" drives can't. As a result, you need to
explicitly tell FluxEngine on the command line whether you're using a high
density disk or not with the `--hd` flag.
**If you don't do this, your disks may not read correctly and will _certainly_
fail to write correctly.**

You can distinguish high density 5.25" floppies from the presence of a
traction ring around the hole in the middle of the disk; if the ring is not
present, the disk is probably high density. However, this isn't always the
case, and reading the disk label is much more reliable.

[Lots more information on high density vs double density disks can be found
here.](http://www.retrotechnology.com/herbs_stuff/guzis.html)

### Other important flags

These flags apply to many operations and are useful for modifying the overall
behaviour.

  - `--revolutions=X`: when reading, spin the disk X times. X can be a floating
	point number. The default is usually 1.25. Some formats default to 1.
	Increasing the number will sample more data, and can be useful on dubious
	disks to try and get a better read.

  - `--sync-with-index=true|false`: wait for an index pulse before starting to
	read the disk. (Ignored for write operations.) By default FluxEngine
	doesn't, as it makes reads faster, but when diagnosing disk problems it's
	helpful to have all your data start at the same place each time.

  - `--index-source=X`, `--write-index-source=X`: set the source of index
	pulses when reading or writing respectively. This is for use with drives
	which don't produce index pulse data. Use 0 to get index pulses from the
	drive, 1 to fake 300RPM pulses, or 2 to fake 360RPM pulses. Note this has
	no effect on the _drive_, so it doesn't help with flippy disks, but is
	useful for using very old drives with FluxEngine itself. If you use this
	option, then any index marks in the sampled flux are, of course, garbage.

### The commands

The FluxEngine client software is a largely undocumented set of small tools.
You'll have to play with them. They all support `--help`. They're not
installed anywhere and after building you'll find them in the `.obj`
directory.

  - `fluxengine erase`: wipes (all or part of) a disk --- erases it without
	writing a pulsetrain.

  - `fluxengine inspect`: dumps the raw pulsetrain / bitstream to stdout.
	Mainly useful for debugging.

  - `fluxengine read *`: reads various formats of disk. See the per-format
	documentation linked from the table [in the index page](../README.md).
	These all take an optional `--write-flux` option which will cause the raw
	flux to be written to the specified file as well as the normal decode.
	There are various `--dump` options for showing raw data during the decode
	process, and `--write-csv` will write a copious CSV report of the state of
	every sector in the file in machine-readable format.

  - `fluxengine write *`: writes various formats of disk. Again, see the
	per-format documentation [in the index page](../README.md).

  - `fluxengine writeflux`: writes raw flux files. This is much less useful
	than you might think: you can't reliably write flux files read from a disk
	to another disk. (See the [FAQ](faq.md) for more information.) It's mainly
	useful for flux files synthesised by the other `fluxengine write` commands.

  - `fluxengine writetestpattern`: writes regular pulses (at a configurable
	interval) to the disk. Useful for testing drive jitter, erasing disks in a
	more secure fashion, or simply debugging. Goes well with `fluxengine
	inspect`.

  - `fluxengine rpm`: measures the RPM of the drive (requires a disk in the
	drive). Mainly useful for testing.

  - `fluxengine seek`: moves the head. Mainly useful for finding out whether
	your drive can seek to track 82. (Mine can't.)

  - `fluxengine test bandwidth`: measures your USB throughput.  You don't need
	a disk in the drive for this one.

  - `fluxengine test voltages`: measures your FDD bus signal voltages, which is
	useful for testing for termination issues.

  - `fluxengine upgradefluxfile`: occasionally I need to upgrade the flux file
	format in a non-backwards-compatible way; this tool will upgrade flux files
	to the new format.

  - `fluxengine convert`: converts flux files from various formats to various
	other formats. You can use this to convert Catweasel flux files to
	FluxEngine's native format, FluxEngine flux files to various other formats
	useful for debugging (including VCD which can be loaded into
	[sigrok](http://sigrok.org)), and bidirectional conversion to and from
	Supercard Pro `.scp` format.

	**Important SCP note:** import (`fluxengine convert scptoflux`) should be
	fairly robust, but export (`fluxengine convert fluxtoscp`) should only be
	done with great caution as FluxEngine files contain features which can't be
	represented very well in `.scp` format and they're probably pretty dubious.
	As ever, please [get in
	touch](https://github.com/davidgiven/fluxengine/issues/new) with any
	reports.

Commands which normally take `--source` or `--dest` get a sensible default if
left unspecified. `fluxengine read ibm` on its own will read drive 0 and
write an `ibm.img` file.

## Visualisation

When doing a read (either from a real disk or from a flux file) you can use
`--write-svg=output.svg` to write out a graphical visualisation of where the
sectors are on the disk. Here's a IBM PC 1232kB disk:

![A disk visualisation](./visualiser.svg)

Blue represents data, light blue a header, and red is a bad sector. Side zero
is on the left and side one is on the right.

The visualiser is extremely primitive and you have to explicitly tell it how
big your disk is, in milliseconds. The default is 200ms (for a normal 3.5"
disk). For a 5.25" disk, use `--visualiser-period=166`.

## Extra programs

Supplied with FluxEngine, but not part of FluxEngine, are some little tools I
wrote to do useful things. These are built alongside FluxEngine.

  - `brother120tool`, `brother240tool`: does things to Brother word processor
	disks. These are [documented on the Brother disk format
	page](disk-brother.md).
  
## The recommended workflow

So you've just received, say, a huge pile of old Brother word processor disks
containing valuable historical data, and you want to read them.

Typically I do this:

```
$ fluxengine read brother -s :d=0 -o brother.img --write-flux=brother.flux --overwrite --write-svg=brother.svg
```

This will read the disk in drive 0 and write out a filesystem image. It'll also
copy the flux to `brother.flux` (replacing any old one) and write out an SVG
visualisation. If I then need to tweak the settings, I can rerun the decode
without having to physically touch the disk like this:

```
$ fluxengine read brother -s brother.flux -o brother.img --write-svg=brother.svg
```

Apart from being drastically faster, this avoids touching the (potentially
physically fragile) disk.

If the disk is particularly dodgy, you can force FluxEngine not to retry
failed reads with `--retries=0`. This reduces head movement. **This is not
recommended.** Floppy disks are inherently unreliable, and the occasional bit
error is perfectly normal; FluxEngine will retry and the sector will read
fine next time. If you prevent retries, then not only do you get bad sectors
in the resulting image, but the flux file itself contains the bad read, so
attempting a decode of it will just reproduce the same bad data.

See also the [troubleshooting page](problems.md) for more information about
reading dubious disks.
