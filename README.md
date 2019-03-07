FluxEngine
==========

What?
-----

The FluxEngine is a very cheap USB floppy disk interface capable of reading and
writing exotic non-PC floppy disk formats. It allows you to use a conventional
PC drive to accept Amiga disks, CLV Macintosh disks, bizarre 128-sector CP/M
disks, and other weird and bizarre formats. (Although not all of these are
supported yet. I could really use samples.)

<img src="floppy.jpg" style="width:50%" alt="a FluxEngine attached to a floppy drive">

**Important note.** On 2019-02-09 I did a hardware redesign and moved the pins on
the board. Sorry for the inconvenience, but it means you don't have to modify
the board any more to make it work. If you built the hardware prior to then,
you'll need to adjust it.

**New!** There's a demo reel!

<div style="text-align: center">
<iframe width="560" height="315" src="https://www.youtube.com/embed/m_s1iw8eW7o" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
</div>

### Infrequently asked questions because nobody's got round to asking them yet

**Q.** Why not just use a USB floppy drive? There are lots and they're cheap.

**A.** Because USB floppy drives typically support a very limited set of
formats --- typically only IBM 1440kB and 720kB. The FluxEngine should work
on (almost) anything.

**Q.** But aren't floppy disks obsolete?

**A.** Absolutely they are. That doesn't mean they've gone away. Good luck
with any old hardware, for example; a classic Mac won't boot without a
classic Mac boot disk, and you can't make them on PCs (because they're
weird). This is where the FluxEngine comes in.

**Q.** But how can you read and write non-PC formats on a PC floppy drive?

**A.** Because the FluxEngine hardware simply streams the raw magnetic flux
pulsetrain from the drive to the PC, and then the analysis is done off-line
in software. It doesn't rely on any floppy disk controller to interpret the
pulsetrain, so we can be a lot cleverer. In fact, the disk doesn't even have
to be spinning at the same speed.

**Q.** Does it work on 5.25" drives?

**A.** Yes! Although PC 5.25" drives spin at 360 RPM rather than 300 RPM,
which means there's only 166ms of data on one per track rather than 200ms;
if you try to write a 3.5" format disk onto one it probably won't work.

**Q.** Does it work on 8" drives?

**A.** Probably? You'd need an adapter to let you connect the drive to the
FluxEngine --- [you can get them](http://www.dbit.com/fdadap.html). I don't
have either the adapter, the drive, or any 8" disks. If anyone wants to give
it a try, please [tell me about
it](https://github.com/davidgiven/fluxengine/issues/new).

**Q.** Is this like KryoFlux? Do you support KryoFlux stream files?

**A.** It's very like KryoFlux, although much simpler. Yes, FluxEngine can
read from KryoFlux stream files (but not write to them yet; nobody's asked).
FluxEngine doesn't capture all the data that KryoFlux does, like index
markers.

**Q.** I've tried it and my disk doesn't work!

**A.** [I have an entire page on diagnosing read failures.](doc/problems.md)

**Q.** That's awesome! What formats does it support?

**A.** I'm glad you asked the question. Consult the following table!

### Formats that it supports

Here's the table.

| Format                                 | Read? | Write? | Notes |
|:---------------------------------------|:-----:|:------:|-------|
| IBM PC compatible                      |  🦄   |        | and compatibles (like the Atari ST) |
| [Acorn ADFS](doc/disk-acornadfs.md)      |  🦄   |        | single- and double- sided           |
| [Acorn DFS](doc/disk-acorndfs.md)        |  🦄   |        |                                     |
| [AES Superplus / No Problem](doc/disk-aeslanier.md) |  🦖   | | hard sectors! and _very_ experimental |
| [Ampro Little Board](doc/disk-ampro.md)  |  🦖   |        |                                     |
| [Apple II DOS 3.3](doc/disk-apple2.md)   |  🦖   |        | doesn't do logical sector remapping |
| [Amiga](doc/disk-amiga.md)               |  🦄   |        |                                     |
| [Commodore 64 1541](doc/disk-c64.md)     |  🦖   |        | and probably the other GCR formats  |
| [Brother 120kB](doc/disk-brother.md)     |  🦄   |        |                                     |
| [Brother 240kB](doc/disk-brother.md)     |  🦄   |   🦄   |                                     |
| [Macintosh 800kB](doc/disk-macintosh.md) |  🦖   |        | and probably the 400kB too          |
| [TRS-80](doc/disk-trs80.md)              |  🦖   |        | a minor variation of the IBM scheme |
| [Victor 9000](doc/disk-victor9k.md)      |  🦖   |        | experimental, probably buggy        |
{: .datatable }

Dinosaurs (🦖) have yet to be observed in real life --- I've written the
decoder based on Kryoflux (or other) dumps I've found. I don't (yet) have
real, physical disks in my hand to test the capture process.

Unicorns (🦄) are completely real --- this means that I've read actual,
physical disks with these formats and so know they work.

Notes:

  - IBM PC disks are the lowest-common-denominator standard. A number of other
    systems use this format in disguise (the Atari ST, late-era Apple
    machines, Acorn). FluxEngine supports both FM and MFM disks, although you
    have to tell it which one. If you have an unknown disk, try this; you may
    get something. Then [tell me about
    it](https://github.com/davidgiven/fluxengine/issues/new).

  - Not many formats support writing yet. That's because I need actual,
    physical hardware to test with in order to verify it works, and I only
    have a limited selection. (Plus a lot of the write code needs work.)
    There hasn't been a lot of demand for this yet; if you have a pressing
    need to write weird disks, [please
    ask](https://github.com/davidgiven/fluxengine/issues/new). I haven't
    implement write support for PC disks because they're boring and I'm lazy,
    and also because they vary so much that figuring out how to specify them
    is hard.

If you have samples of weird disks, and want to send them to me --- either
FluxEngine or Kryoflux dumps, or (even better) actually physically --- I can
identify them and add support.

Please note that at this point I am *not interested in copy protected disks*.
It's not out of principle. It's just they'll drive me insane. FluxEngine will
most likely be able to read the data fine, unless they're doing bizarre
things like spiral tracks or partially encoded data, but let's stick with
normal conventionally formatted disks for the time being!

### Big list of stuff to work on

Both the firmware and the software are works in progress. There's still things
to do.

  - Support for more floppy disk formats! I'm trying to scare up some sample
	disks to image and decode; I'm particularly looking for Commodore 64 1541
	disks and Apple Macintosh 800kB CLV disks. If you have any which I can
	borrow and you live in (or near) Switzerland, please [get in
	touch](https://github.com/davidgiven/fluxengine/issues/new).

  - Better (and more) write support. The hardware can write disks, but not much
	of the software's done.  Writing is hard because I really need the device
	to test the disks on. I could use some help here. Also, most of the write
	code is a bit of a disaster and needs a major refactoring.

  - Sourcing a different microcontroller. The PSoC5 is a great thing to work
	with, but it's disappointingly proprietary and the toolchain only works on
	Windows. It'd be nice to support something easier to work with. I need a 5V
	microcontroller (which unfortunately rules out Arduinos) with at least
	seventeen GPIO pins in a row.  As non-PSoC5 microcontrollers won't have the
	FPGA soft logic, that also means I'd most likely need to bitbang the floppy
	drive, so speed is important. If you have any recommendations, please [get
	in touch](https://github.com/davidgiven/fluxengine/issues/new).

Where?
------

It's [open source on GitHub!](https://github.com/davidgiven/fluxengine)

How?
----

### Introduction

The system is based around an off-the-shelf Cypress development board,
costing about $10 plus shipping. The only hardware modification you need to
do is to attach a floppy drive connector.

[I wrote a long, detailed set of instructions on building and programming
it.](doc/building.md)

I'm going to assume you've done this.

### Using it

  1. Attach the FluxEngine to your floppy disk cable.

     If you built your board using a connector, this is easy: just plug it in. If you're using pins, you need to make sure that the red strip on the cable is **to the right** (next to pin 2.7 on the board), and that the pins plug into the **lower set of holes** in the connector (so the connector overhangs the top of the board).

  2. Attach the cable to your drives.

     Floppy disk cables typically have [two pairs of floppy disk drive
     connectors with a twist between
     them](http://www.nullmodem.com/Floppy.htm). (Each pair has one connector
     for a 3.5" drive and a different one for a 5.25" drive.) Normally the
     first floppy disk drive is drive B, and the one after the twist is drive
     A. However, FluxEngine considers the first drive to be drive 0 and the
     second one to be drive 1. It's this way so as to allow the FluxEngine to
     be plugged directly onto the back of a disk drive. If you only have one
     drive, remember to plug the drive into the connector _before_ the twist.
     (Or use `-s :d=1` to select drive 1 when working with disks.)

  2. **Important.** Make sure that no disk you care about is in the drive.
     (Because if your wiring is wrong and a disk is inserted, you'll probably
     corrupt it.)

  3. Connect the floppy drive to power. Nothing should happen. If anything
     does, disconnect it and check step 1.

  4. Connect the FluxEngine to your PC via USB --- using the little socket on
     the board, not the big programmer plug.

  5. Do `.obj/fe-rpm` from the shell. The motor should work and it'll tell you
     that the disk is spinning at about 300 rpm. If it doesn't, please [get
     in touch](https://github.com/davidgiven/fluxengine/issues/new).

  6. Insert a standard PC formatted floppy disk into the drive (probably a good
     idea to remove the old disk first). Then do `.obj/fe-readibm`. It should
     read the disk, emitting copious diagnostics, and spit out an `ibm.img`
     file containing the decoded disk image (either 1440kB or 720kB depending).

  7. Success!

### The programs

#### Source and destination specifiers

When reading from or writing to _a disk_ (or a file pretending to be a disk),
use the `-s` and `-d` options to tell FluxEngine which bits of the disk you
want to access. These use a common syntax:

```
fe-readibm -s fakedisk.flux:t=0-79:s=0
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
    linked from the table above.

  - `fe-write*`: writes various formats of disk. Again, see the per-format
    documentation above.

  - `fe-writeflux`: writes raw flux files. This is much less useful than you
    might think: you can't necessarily copy flux files read from a disk,
    because errors in the sampling are compounded and the result probably
    isn't readable. It's mainly useful for flux files synthesised by the
    other `fe-write*` commands.

  - `fe-writetestpattern`: writes regular pulses (at a configurable interval)
    to the disk. Useful for testing drive jitter, erasing disks in a more
    secure fashion, or simply debugging. Goes well with `fe-inspect`.

  - `fe-readibm`: reads 720kB or 1440kB IBM MFM disks. Emits a standard
    filesystem image.

  - `fe-rpm`: measures the RPM of the drive (requires a disk in the drive).
    Mainly useful for testing.

  - `fe-seek`: moves the head. Mainly useful for finding out whether your drive
    can seek to track 82. (Mine can't.)

  - `fe-testbulktransport`: measures your USB throughput. You need about 600kB/s
    for FluxEngine to work. You don't need a disk in the drive for this one.

  - `fe-upgradefluxfile`: occasionally I need to upgrade the flux file format in
    a non-backwards-compatible way; this tool will upgrade flux files to the new
    format.

Commands which take `--source` or `--dest` take a parameter of the syntax
`$FILENAME:$MODIFIER:$MODIFER...` as described above. If left unspecified,
you get the default specified by the command, which will vary depending on
which disk format you're using (and is usually the right one).

### How it works

It's very very simple. The firmware measures the time between flux transition
pulses, encodes them, and streams them over USB to the PC.

There's an 8-bit counter attached to an 12MHz clock. This is used to measure
the interval between pulses. If the timer overflows, we pretend it's a pulse
(this very rarely happens in real life). ([I'm working on
this.](https://github.com/davidgiven/fluxengine/issues/8))

An HD floppy has a nominal clock of 500kHz, so we use a sample clock of 12MHz
(every 83ns). This means that our 500kHz pulses will have an interval of 24
(and a DD disk with a 250kHz nominal clock has an interval of 48). This gives
us more than enough resolution. If no pulse comes in, then we sample on
rollover at 21us.

(The clock needs to be absolutely rock solid or we get jitter which makes the
data difficult to analyse, so 12 was chosen to be derivable from the
ultra-accurate USB clock.)

Once at the PC, we do some dubious heuristics to determine the clock rate,
which depends on what kind of encoding is being used, and that in turn lets
us decode the pulsetrain into actual bits and derive the raw floppy disk
records from there. Reassembling the image and doing stuff like CRC checking
is straightforward.

Some useful and/or interesting numbers:

  - nominal rotation speed is 300 rpm, or 5Hz. The period is 200ms.
  - a pulse is 150ns to 800ns long.
  - a 12MHz tick is 83ns.
  - MFM HD encoding uses a clock of 500kHz. This makes each recording cell 2us,
    or 24 ticks. For DD it's 4us and 48 ticks.
  - a short transition is one cell (2us == 24 ticks). A medium is a cell and
    a half (3us == 36 ticks). A long is two cells (4us == 48 ticks). Double
    that for DD.
  - pulses are detected with +/- 350ns error for HD and 700ns for DD. That's
    4 ticks and 8 ticks. That seems to be about what we're seeing.
  - in real life, start going astray after about 128 ticks == 10us. If you
    don't write anything, you read back random noise.
  
Useful links:

  - [The floppy disk user's
    guide](http://www.hermannseib.com/documents/floppy.pdf): an incredibly
    useful compendium of somewhat old floppy disk information --- which is
    fine, because floppy disks are somewhat old.

  - [The TEAC FD-05HF-8830 data
    sheet](https://hxc2001.com/download/datasheet/floppy/thirdparty/Teac/TEAC%20FD-05HF-8830.pdf):
    the technical data sheet for a representative drive. Lots of useful
    timing numbers here.

  - [KryoFlux stream file
    documentation](https://www.kryoflux.com/download/kryoflux_stream_protocol_rev1.1.pdf):
    the format of KryoFlux stream files (partially supported by FluxEngine)

Who?
----

The FluxEngine was designed, built and written by me, David Given. You may
contact me at dg@cowlark.com, or visit my website at http://www.cowlark.com.
There may or may not be anything interesting there.

License
-------

Everything here is licensed under the MIT open source license. Please see
[COPYING](COPYING) for the full text. The tl;dr is: you can do what you like
with it provided you don't claim you wrote it.
