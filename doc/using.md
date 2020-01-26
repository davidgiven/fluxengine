Using a FluxEngine
==================

So you've [built the hardware](building.md)! What now?

## Connecting it up

In order to do anything useful, you have to plug it in to a floppy disk drive (or two).

  1. Plug the motherboard end of your floppy disk cable into the FluxEngine.
     
     The **red stripe goes on the right**. The **lower set of
     holes connect to the board**. (Pin 2 of the connector needs to connect
     to pin 2.7 on the board.)

     If you're using header pins, the upper row of holes in the connector
     should overhang the edge of the board. If you're using a floppy drive
     motherboard connector, you're golden, of course (unless you have one of
     those annoying unkeyed cables, or have accidentally soldered the
     connector on in the wrong place --- don't laugh, I've done it.)

  2. Plug the drive end of your floppy disk cable into the drive (or drives).

     Floppy disk cables typically have [two pairs of floppy disk drive
     connectors with a twist between
     them](http://www.nullmodem.com/Floppy.htm). (Each pair has one connector
     for a 3.5" drive and a different one for a 5.25" drive.) (Some cables
     are cheap and just have the 3.5" connectors. Some are _very_ cheap and
     have a single 3.5" connector, after the twist.)
     
     FluxEngine uses, sadly, non-standard disk numbering (there are reasons).
     Drive 0 is the one nearest the motherboard; that is, before the twist.
     Drive 1 is the one at the end of the cable; that is, after the twist.
     Drive 0 is the default. If you only have one drive, remember to plug the
     drive into the connector _before_ the twist. (Or use `-s :d=1` to select
     drive 1 when working with disks.)

  3. **Important.** Make sure that no disk you care about is in the drive.
     (Because if your wiring is wrong and a disk is inserted, you'll corrupt it.)

  4. Connect the floppy drive to power. Nothing should happen. If you've
     connected something in backwards, you'll see the drive light up, the
     motor start, and if you didn't take the disk out, one track has just
     been wiped. If this happens, check your wiring.

  5. Connect the FluxEngine to your PC via USB --- using the little socket on
     the board, not the big programmer plug.

  6. Insert a scratch disk and do `fluxengine rpm` from the shell. The motor
     should work and it'll tell you that the disk is spinning at about 300
     rpm for a 3.5" disk, or 360 rpm for a 5.25" disk. If it doesn't, please
     [get in touch](https://github.com/davidgiven/fluxengine/issues/new).

  7. Do `fluxengine test bulktransport` from the shell. It'll measure your USB
     bandwidth. Ideally you should be getting above 900kB/s. FluxEngine needs
     about 850kB/s, so if you're getting less than this, try a different USB
     port.

  8. Insert a standard PC formatted floppy disk into the drive (probably a good
     idea to remove the old disk first). Then do `fluxengine read ibm`. It
     should read the disk, emitting copious diagnostics, and spit out an
     `ibm.img` file containing the decoded disk image (either 1440kB or 720kB
     depending).

  9. Profit!

## Bonus hardware features

For advanced users, the board has a few extra signals which are useful for special purposes.

  - Pin 3[0] produces short pulses every 200ms. This is useful for spoofing
    index signals to 300 RPM drives; for example, to read flippy disks.

  - Pin 3[1] is the same, but produces the pulses every 166ms; this works with
    360 RPM drives.

## The programs

I'm sorry to say that the client program is very badly documented --- it's
moving too quickly for the documentation to keep up. It does respond to
`--help` or `help` depending on context. There are some common properties,
described below.

### Source and destination specifiers

When reading from or writing to _a disk_ (or a file pretending to be a disk),
use the `--source` (`-s`) and `--dest` (`-d`) options to tell FluxEngine
which bits of the disk you want to access. These use a common syntax:

```
fluxengine read ibm -s fakedisk.flux:t=0-79:s=0
```

  - To access a real disk, leave out the filename (so `:t=0-79:s=0`).

  - To access only some tracks, use the `t=` modifier. To access only some
    sides, use the `s=` modifier. To change drives, use `d=`.

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
*disk format says otherwise).

### Input and output specifiers

These use a very similar syntax to the source and destination specifiers
(because they're based on the same microformat library!) but are used for
input and output _images_: i.e. nicely lined up arrays of sectors which you
can actually do something with.

Use `--input` (`-i`) or `--output` (`-o`) as appropriate to tell FluxEngine
where you want to read from or write to. The actual format is autodetected
based on the extension:

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

  - `--revolutions=X`: when reading, spin the disk X times. Many formats
  require `--revolutions=2` (which should happen automatically); or you can
  increase the number to sample more data.

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
  documentation linked from the table above. These all take an optional
  `--write-flux` option which will cause the raw flux to be written to the
  specified file. There are various `--dump` options for showing raw data
  during the decode process.

  - `fluxengine write *`: writes various formats of disk. Again, see the
  per-format documentation above.

  - `fluxengine writeflux`: writes raw flux files. This is much less useful
  than you might think: you can't write flux files read from a disk to
  another disk. (See the [FAQ](faq.md) for more information.) It's mainly
  useful for flux files synthesised by the other `fluxengine write` commands.

  - `fluxengine writetestpattern`: writes regular pulses (at a configurable
  interval) to the disk. Useful for testing drive jitter, erasing disks in a
  more secure fashion, or simply debugging. Goes well with `fluxengine
  inspect`.

  - `fluxengine rpm`: measures the RPM of the drive (requires a disk in the
  drive). Mainly useful for testing.

  - `fluxengine seek`: moves the head. Mainly useful for finding out whether
  your drive can seek to track 82. (Mine can't.)

  - `fluxengine test bulktransport`: measures your USB throughput. You need
  about 600kB/s for FluxEngine to work. You don't need a disk in the drive
  for this one.

  - `fluxengine test voltages`: measures your FDD bus signal voltages, which
  is useful for testing for termination issues.

  - `fluxengine upgradefluxfile`: occasionally I need to upgrade the flux
  file format in a non-backwards-compatible way; this tool will upgrade flux
  files to the new format.

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
  touch](https://github.com/davidgiven/fluxengine/issues/new) with any reports.

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

  - `brother120tool`: extracts files from a 120kB Brother filesystem image.

## The recommended workflow

So you've just received, say, a huge pile of old Brother word processor disks
containing valuable historical data, and you want to read them.

Typically I do this:

```
$ fluxengine read brother -s :d=0 -o brother.img --write-flux=brother.flux --write-svg=brother.svg
```

This will read the disk in drive 0 and write out a filesystem image. It'll
also copy the flux to brother.flux and write out an SVG visualisation. If I
then need to tweak the settings, I can rerun the decode without having to
physically touch the disk like this:

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
