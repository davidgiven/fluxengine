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

  6. Insert a scratch disk and do `.obj/fe-rpm` from the shell. The motor
     should work and it'll tell you that the disk is spinning at about 300
     rpm for a 3.5" disk, or 360 rpm for a 5.25" disk. If it doesn't, please
     [get in touch](https://github.com/davidgiven/fluxengine/issues/new).

  7. Do `.obj/fe-testbulktransport` from the shell. It'll measure your USB
     bandwidth. Ideally you should be getting above 900kB/s. FluxEngine needs
     about 850kB/s, so if you're getting less than this, try a different USB
     port.

  8. Insert a standard PC formatted floppy disk into the drive (probably a good
     idea to remove the old disk first). Then do `.obj/fe-readibm`. It should
     read the disk, emitting copious diagnostics, and spit out an `ibm.img`
     file containing the decoded disk image (either 1440kB or 720kB depending).

  9. Profit!

## The programs

I'm sorry to say that the programs are very badly documented --- they're
moving too quickly for the documentation to keep up. They do all respond to
`--help`. There are some common properties, described below.

### Source and destination specifiers

When reading from or writing to _a disk_ (or a file pretending to be a disk),
use the `--source` (`-s`) and `--dest` (`-d`) options to tell FluxEngine
which bits of the disk you want to access. These use a common syntax:

```
.obj/fe-readibm -s fakedisk.flux:t=0-79:s=0
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

### The commands

The FluxEngine client software is a largely undocumented set of small tools.
You'll have to play with them. They all support `--help`. They're not
installed anywhere and after building you'll find them in the `.obj`
directory.

  - `fe-erase`: wipes (all or part of) a disk --- erases it without writing
    a pulsetrain.

  - `fe-inspect`: dumps the raw pulsetrain / bitstream to stdout. Mainly useful
    for debugging.

  - `fe-read*`: reads various formats of disk. See the per-format documentation
    linked from the table above. These all take an optional `--write-flux`
    option which will cause the raw flux to be written to the specified file.

  - `fe-write*`: writes various formats of disk. Again, see the per-format
    documentation above.

  - `fe-writeflux`: writes raw flux files. This is much less useful than you
    might think: you can't write flux files read from a disk to another disk.
    (See the [FAQ](faq.md) for more information.) It's mainly useful for flux
    files synthesised by the other `fe-write*` commands.

  - `fe-writetestpattern`: writes regular pulses (at a configurable interval)
    to the disk. Useful for testing drive jitter, erasing disks in a more
    secure fashion, or simply debugging. Goes well with `fe-inspect`.

  - `fe-rpm`: measures the RPM of the drive (requires a disk in the drive).
    Mainly useful for testing.

  - `fe-seek`: moves the head. Mainly useful for finding out whether your drive
    can seek to track 82. (Mine can't.)

  - `fe-testbulktransport`: measures your USB throughput. You need about 600kB/s
    for FluxEngine to work. You don't need a disk in the drive for this one.

  - `fe-upgradefluxfile`: occasionally I need to upgrade the flux file format in
    a non-backwards-compatible way; this tool will upgrade flux files to the new
    format.

Commands which normally take `--source` or `--dest` get a sensible default if left
unspecified. `fe-readibm` on its own will read drive 0 and write an `ibm.img` file.

## Extra programs

Supplied with FluxEngine, but not part of FluxEngine, are some little tools I
wrote to do useful things. These are built alongside FluxEngine.

  - `brother120tool`: extracts files from a 120kB Brother filesystem image.

  - `cwftoflux`: converts (one flavour of) CatWeasel flux file into a
    FluxEngine flux file.

## The recommended workflow

So you've just received, say, a huge pile of old Brother word processor disks containing valuable historical data, and you want to read them.

Typically I do this:

```
$ fe-readbrother -s :d=0 -o brother.img --write-flux=brother.flux
```

This will read the disk in drive 0 and write out a filesystem image. It'll also copy the flux to brother.flux. If I then need to tweak the settings, I can rerun the decode without having to physically touch the disk like this:

```
$ fe-readbrother -s brother.flux -o brother.img
```

If the disk is particularly fragile, you can force FluxEngine not to retry
failed reads with `--retries=0`. This reduces head movement. **This is not
recommended.** Floppy disks are inherently unreliable, and the occasional bit
error is perfectly normal; the sector will read fine next time. If you
prevent retries, then not only do you get bad sectors in the resulting image,
but the flux file itself contains the bad read, so attempting a decode of it
will just reproduce the same bad data.
