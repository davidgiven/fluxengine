Using a FluxEngine
==================

So you've [built the hardware](building.md), programmed and tested it! What
now?

## The programs

I'm sorry to say that the client program is very badly documented --- it's
moving too quickly for the documentation to keep up. It does respond to
`--help` or `help` depending on context. There are some common properties,
described below.

If possible, try using the GUI, which should provide simplified access for most
common operations.

<div style="text-align: center">
<a href="doc/screenshot-details.png"><img src="doc/screenshot-details.png" style="width:60%" alt="screenshot of the GUI in action"></a>
</div>

### Core concepts

FluxEngine's job is to read magnetic data (called _flux_) off a disk, decode
it, and emit a filesystem image (called, er, an _image_); or, the other way
round, where an image is read, converted to flux, and written to a disk.

A file system image typically has the extension `.img`. It contains a
sector-by-sector record of the _decoded_ data on the disk. For example, on a
disk with 512 byte sectors, one sector will occupy 512 bytes. These are
typically what you want in everyday life. FluxEngine supports a variety of file
system image formats, including
[LDBS](http://www.seasip.info/Unix/LibDsk/ldbs.html), [imd](http://dunfield.classiccmp.org/img/index.htm),
 Macintosh's [DiskCopy 4.2](https://en.wikipedia.org/wiki/Disk_Copy) and some others, including
Amiga's `.adf`, Atari ST's `.st`, and so on.

Flux, however, is different. It represents the actual magnetic data on the
disk. This has to be decoded before anything useful can be done with it (like
turning it into a file system image). It's possible to read the flux data off
the disk and write it to a file, known as a _flux file_, which allows you to
fiddle with the decode parameters without having to touch the disk again, which
might be fragile. FluxEngine supports several different kinds of flux file,
including its own, SuperCard Pro's `.scp` format, and the Kryoflux stream
format. A flux file will typically contain from 80 to 150 kilobytes of data
per track.

In general, FluxEngine can use either a real disk or a flux file
interchangeably: you can specify either at (very nearly) any time. A very
common workflow is to read a disk to a flux file, and then reread from the flux
file while changing the decoder options, to save disk wear. It's also much faster.

### Connecting it up

To use, simply plug your FluxEngine (or [Greaseweazle](greaseweazle.md) or
[Applesauce](applesauce.md)) into your computer and run the client. If a single
device is plugged in, it will be automatically detected and used.

If _more_ than one device is plugged in, you need to specify which one to use
with the `--usb.serial` parameter, which takes the device serial number as a
parameter.  You can find out the serial numbers by running the command without
the `--usb.serial` parameter, and if more than one device is attached they will
be listed. The serial number is also shown whenever a connection is made. You
can list all the detectable devices with:

```
$ fluxengine test devices
```

This will show you their serial numbers.

You _can_ work with more than one FluxEngine at the same time, using different
invocations of the client; but be careful of USB bandwidth. If the devices are
connected via the same hub, the bandwidth will be shared.

### Basic use

The FluxEngine client is a command line program. As parameters it takes one or
more words telling it what to do, and then a bunch of configuration options.
Configurations can be specified either on the command line or in text files.

Here are some sample invocations:

```
# Read an PC 1440kB disk, producing a disk image with the default name
# (ibm.img)
$ fluxengine read ibm --1440

# Write a PC 1440kB disk to drive 1
$ fluxengine write ibm --1440 -i image.img -d drive:1

# Read a Eco1 CP/M disk, making a copy of the flux into a file
$ fluxengine read eco1 --copy-flux-to copy.flux -o eco1.ldbs

# Rerun the decode from the flux file, tweaking the parameters
$ fluxengine read eco1 -s copy.flux -o eco1.ldbs --cylinders=1
```

### Configuration

Configuration options are represented as a hierarchical structure. You can
either put them in a text file and load them from the command line:

```
$ cat config.textpb
encoder {
  ibm {
    trackdata {
      emit_iam: false
    }
  }
}
$ fluxengine write ibm --1440 config.textpb -i image.img
```

...or you can specify them on the command line:

```
$ fluxengine write ibm --1440 -i image.img --encoder.ibm.trackdata.emit_iam=false
```

Both the above invocations are equivalent. The text files use [Google's
protobuf syntax](https://developers.google.com/protocol-buffers), which is
hierarchical, type-safe, and easy to read.

The `ibm` string above is actually a reference to an internal configuration file
containing all the settings for writing PC disks, and the `--1140` refers to a
specific definition inside it. You may specify as many profile names or textpb
files as you wish; they are all merged left to right.  You can see all these
settings by doing:

```
$ fluxengine write ibm --1440 --config
```

The `--config` option will cause the current configuration to be dumped to the
console, and then the program will halt.

Going into the details of the configuration is complicated and mostly futile as
it's likely to change as things get modified. Brief but up-to-date information
about each configuration setting is available with the `--doc` option. Note
that not all combinations of settings make sense.

### The tools

The FluxEngine program has got multiple sub-tools, each of which performs a
different task. Run each one with `--help` to get a full list of
(non-configuration-setting) options; this describes only basic usage of the
more common tools.

  - `fluxengine read <profile> <options> -s <flux source> -o <image output>`

    Reads flux (possibly from a disk) and decodes it into a file system image.
    `<profile>` is a reference to an internal input configuration file
    describing the format. `<options>` may be any combination of options
    defined by the profile.

  - `fluxengine write <profile> -i <image input> -d <flux destination>`

    Reads a filesystem image and encodes it into flux (possibly writing to a
    disk). `<profile>` is a reference to an internal output configuration file
    describing the format.

  - `fluxengine rawread -s <flux source> -d <flux destination>`

    Reads flux (possibly from a disk) and writes it to a flux file without doing
    any decoding. You can specify a profile if you want to read a subset of the
    disk.

  - `fluxengine rawwrite -s <flux source> -d <flux destination>`

    Reads flux from a file and writes it (possibly to a disk) without doing any
    encoding. You can specify a profile if you want to write a subset of the
    disk.

  - `fluxengine merge -s <fluxfile> -s <fluxfile...> -d <fluxfile`

    Merges data from multiple flux files together. This is useful if you have
    several reads from an unreliable disk where each read has a different set
    of good sectors. By merging the flux files, you get to combine all the
    data. Don't use this on reads of different disks, for obvious results! Note
    that this works on flux files, not on flux sources.

  - `fluxengine inspect -s <flux source> -c <cylinder> -h <head> -B`

    Reads flux (possibly from a disk) and does various analyses of it to try and
    detect the clock rate, display raw flux information, examine the underlying
    data from the FluxEngine board, etc. There are lots of options but the
    command above is the most useful.

  - `fluxengine rpm`

    Measures the rotation speed of a drive. For hard-sectored disks, you
    probably want to add the name of a read profile to configure the number of
    sectors.

  - `fluxengine seek -c <cylinder>`

    Seeks a drive to a particular cylinder.

There are other tools; try `fluxengine --help`.

**Important note on `rawread` and `rawwrite`:** You can't use these tools to
copy disks, in most circumstances. See [the FAQ](faq.md) for more information.
Also, `rawread` is not guaranteed to read correctly. Floppy disks are
fundamentally unreliable, and random bit errors may occur at any time; these
can only be detected by performing a decode and verifying the checksums on the
sectors. To perform a correct read, it's recommended to do `fluxengine read`
with the `--copy-flux-to` option, to perform a decode to a filesystem image
while also writing to a flux file.

### Flux sources and destinations

FluxEngine supports a number of ways to get or put flux. When using the `-s` or
`-d` options (for source and destination), you can use any of these strings:

  - `drive:<n>`

    Read from or write to a specific drive.
  
  - `<filename.flux>`

    Read from or write to a native FluxEngine flux file.
  
  - `<filename.scp>`

    Read from or write to a Supercard Pro `.scp` flux file.
  
  - `<filename.cwf>`

    Read from a Catweasel flux file. **Read only.**
  
  - `dmk:<directory>`

    Read from a Catweasel CMK directory. **Read only.**
  
  - `<filename.a2r>`

    Write to a AppleSauce flux file. **Write only.**

  - `kryoflux:<directory>`

    Read from a Kryoflux stream, where `<path>` is the directory containing
    the stream files. **Read only.**
  
  - `flx:<directory>`

    Read from a FLUXCOPY stream, where `<path>` is the directory containing the
    stream files. **Read only.**

  - `erase:`

    Read nothing --- writing this to a disk will magnetically erase a track.
    **Read only.**
  
  - `testpattern:`

    Read a test pattern, which can be written to a disk to help diagnosis.
    **Read only.**
  
  - `au:<directory>`

    Write to a series of `.au` files, one file per track, which can be loaded
    into an audio editor (such as Audacity) as a simple logic analyser.
    **Write only.**
  
  - `vcd:<directory>`

    Write to a series of `.vcd` files, one file per track, which can be loaded
    into a logic analyser (such as Pulseview) for analysis. **Write only.**

### Image sources and destinations

FluxEngine also supports a number of file system image formats. When using the
`-i` or `-o` options (for input and output), you can use any of these strings:

  - `<filename.adf>`, `<filename.d81>`, `<filename.img>`, `<filename.st>`,
    `<filename.xdf>`

  Read from or write to a simple headerless image file (all these formats are
  the same). This will probably want configuration via the
  `input/output.image.img.*` configuration settings to specify all the
  parameters.
  
  - `<filename.diskcopy>`

  Read from or write to a [DiskCopy
  4.2](https://en.wikipedia.org/wiki/Disk_Copy) image file, commonly used by
  Apple Macintosh emulators.
  
  - `<filename.td0>`

  Read a [Sydex Teledisk TD0
  file](https://web.archive.org/web/20210420224336/http://dunfield.classiccmp.org/img47321/teledisk.htm)
  image file. Note that only uncompressed images are supported (so far).

  - `<filename.jv3>`

  Read from a JV3 image file, commonly used by TRS-80 emulators. **Read
  only.**

  - `<filename.dim>`

  Read from a [DIM image file](https://www.pc98.org/project/doc/dim.html),
  commonly used by X68000 emulators. Supports automatically configuring
  the encoder. **Read only.**
  
  - `<filename.fdi>`

  Read from a [FDI image file](https://www.pc98.org/project/doc/hdi.html),
  commonly used by PC-98 emulators. Supports automatically configuring
  the encoder. **Read only.**
  
  - `<filename.d88>`

  Read from a [D88 image file](https://www.pc98.org/project/doc/d88.html),
  commonly used by various Japanese PC emulators, including the NEC PC-88.
  
  FluxEngine is currently limited to reading only the first floppy image in a
  D88 file. When writing, a single unnamed floppy will be created
  within the image file.
  
  The D88 reader should be used with the `ibm` profile and will override
  most encoding parameters on a track-by-track basis.
  
  The D88 writer should likewise be used with the `ibm` profile in most
  circumstances as it can represent arbitrary sector layouts as read
  from the floppy.
  
  - `<filename.imd>`

  Read from a [imd image file](http://dunfield.classiccmp.org/img/index.htm),
  imd images are entire diskette images read into a file (type .IMD),
  it's purpose is to recreate a copy of the diskette from that image. A detailed analysis
  is performed on the diskette, and information about the formatting is recorded
  in the image file. This allows ImageDisk to work with virtually any soft-
  sectored diskette format that is compatible with the PC's type 765 floppy
  diskette controller and drives.
  
  FluxEngine is able to read from and write to an imd image file.
  
  The imd reader should mostly be used with the `ibm` profile or ibm deratives 
  and will override most encoding parameters on a track-by-track basis.
  
  The imd writer should likewise mostly be used with the `ibm` profile in most
  circumstances as it can represent arbitrary sector layouts as read
  from the floppy.

  With options it is possible to add a comment for the resulting image when archiving
  floppies. By default imd images assume MFM encoding, but this can by changed
  by suppling the option RECMODE_FM. 

  - `<filename.nfd>`

  Read from a [NFD r0 image file](https://www.pc98.org/project/doc/nfdr0.html),
  commonly used by various Japanese PC emulators, including the NEC PC-98. **Read only.**

  Only r0 version files are currently supported.
  
  The NFD reader should be used with the `ibm` profile and will override
  most encoding parameters on a track-by-track basis.
  
  - `<filename.ldbs>`

  Write to a [LDBS generic image
  file](https://www.seasip.info/Unix/LibDsk/ldbs.html). **Write only.**

  - `<filename.d64>`
  
  Write to a [D64 image
  file](http://unusedino.de/ec64/technical/formats/d64.html), commonly used by
  Commodore 64 emulators. **Write only.**

  - `<filename.raw>`

  Write undecoded data to a raw binary file. **Write only.** This gives you the
  underlying MFM, FM or GCR stream, without actually doing the decode into
  user-visible bytes. However, the decode is still done in order to check for
  correctness. Individual records are separated by three `\\0` bytes and tracks
  are separated by four `\\0` bytes; tracks are emitted in CHS order.

### High density disks

High density disks use a different magnetic medium to low and double density
disks, and have different magnetic properties. 3.5" drives can usually
autodetect what kind of medium is inserted into the drive based on the hole in
the disk casing, but 5.25" drives can't. As a result, you need to explicitly
tell FluxEngine on the command line whether you're using a high density disk or
not with the `--drive.high_density` configuration setting.
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

  - `--drive.revolutions=X`

    When reading, spin the disk X times. X can be a floating point number. The
    default is usually 1.2. Some formats default to 1.  Increasing the number
    will sample more data, and can be useful on dubious disks to try and get a
    better read.

  - `--drive.sync_with_index=true|false`

    Wait for an index pulse before starting to read the disk. (Ignored for write
    operations.) By default FluxEngine doesn't, as it makes reads faster, but
    when diagnosing disk problems it's helpful to have all your data start at
    the same place each time.

  - `--drive.index_mode=X`

    Set the source of index pulses when reading or writing respectively. This
    is for use with drives which don't produce index pulse data. `X` can be
    `INDEXMODE_DRIVE` to get index pulses from the drive, `INDEXMODE_300` to
    fake 300RPM pulses, or `INDEXMODE_360` to fake 360RPM pulses.  Note this
    has no effect on the _drive_, so it doesn't help with flippy disks, but is
    useful for using very old drives with FluxEngine itself. If you use this
    option, then any index marks in the sampled flux are, of course, garbage.

## Visualisation

When using `fluxengined read` (either from a real disk or from a flux file) you
can use `--decoder.write_csv_to=output.csv` to write out a CSV file containing
information about the location of every sector on the disk. You can then use
`fluxengine analyse layout` to produce a graphical visualisation of this.
Here's a IBM PC 1232kB disk:

![A disk visualisation](./visualiser.jpg)

Blue represents data, light blue a header, and red is a bad sector. Side zero
is on the left and side one is on the right.

The visualiser is extremely primitive and you have to explicitly tell it how
big your disk is, in milliseconds. The default is 200ms (for a normal 3.5"
disk). For a 5.25" disk, use `--visualiser-period=166`.

## Extra programs

Supplied with FluxEngine, but not part of FluxEngine, are some little tools I
wrote to do useful things. These are built alongside FluxEngine.

  - `brother120tool`, `brother240tool`

  Does things to Brother word processor disks. These are [documented on the
  Brother disk format page](disk-brother.md).
  
## The recommended workflow

So you've just received, say, a huge pile of old Brother word processor disks
containing valuable historical data, and you want to read them.

Typically I do this:

```
$ fluxengine read brother240 -s drive:0 -o brother.img --copy-flux-to=brother.flux --decoder.write_csv_to=brother.csv
```

This will read the disk in drive 0 and write out an information CSV file. It'll
also copy the flux to `brother.flux` (replacing any old one) and write out a
CSV file for used when making a visualisation. If I then need to tweak the
settings, I can rerun the decode without having to physically touch the disk
like this:

```
$ fluxengine read brother -s brother.flux -o brother.img --decoder.write_csv_to=brother.csv
```

Apart from being drastically faster, this avoids touching the (potentially
physically fragile) disk.

If the disk is particularly dodgy, you can force FluxEngine not to retry failed
reads with `--decoder.retries=0`. This reduces head movement. **This is not
recommended.** Floppy disks are inherently unreliable, and the occasional bit
error is perfectly normal; FluxEngine will retry and the sector will read fine
next time. If you prevent retries, then not only do you get bad sectors in the
resulting image, but the flux file itself contains the bad read, so attempting a
decode of it will just reproduce the same bad data.

See also the [troubleshooting page](problems.md) for more information about
reading dubious disks.
