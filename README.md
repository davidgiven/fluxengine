FluxEngine
==========

(If you're reading this on GitHub, the formatting's a bit messed up. [Try the
version on cowlark.com instead.](http://cowlark.com/fluxengine/))

**Breaking news!** As of 2021-05-21, the command line environment has changed
_substantially_ (to make it more consistent and flexible, and allow some new
backend features like multi-format IBM scheme disks, which are popular with
CP/M). If things don't work the way you expect, please check the documentation.

What?
-----

The FluxEngine is a very cheap USB floppy disk interface capable of reading and
writing exotic non-PC floppy disk formats. It allows you to use a conventional
PC drive to accept Amiga disks, CLV Macintosh disks, bizarre 128-sector CP/M
disks, and other weird and bizarre formats. (Although not all of these are
supported yet. I could really use samples.)

The hardware consists of a single, commodity part with a floppy drive
connector soldered onto it. No ordering custom boards, no fiddly surface
mount assembly, and no fuss: nineteen simpler solder joints and you're done.
You can make one for $15 (plus shipping).

Don't believe me? Watch the demo reel!

<div style="text-align: center">
<iframe width="373" height="210" src="https://www.youtube.com/embed/m_s1iw8eW7o" frameborder="0" allow="accelerometer; autoplay; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>
</div>

**New!** The FluxEngine client software now works with
[GreaseWeazle](https://github.com/keirf/Greaseweazle/wiki) hardware. So, if you
can't find a PSoC5 development kit, or don't want to use the Cypress Windows
tools for programming it, you can use one of these instead. Very nearly all
FluxEngine features are available with the GreaseWeazle and it works out-of-the
box. See the [dedicated GreaseWeazle documentation page](doc/greaseweazle.md)
for more information.

**Important note.** On 2020-04-02 I changed the bytecode format (and firmware).
Flux files will need to be upgraded with `fluxengine upgradefluxfile`. The new
format should be more reliable and use way, way less bandwidth. Sorry for the
inconvenience.

Where?
------

It's [open source on GitHub!](https://github.com/davidgiven/fluxengine)

How?
----

This page was getting kinda unwieldy so I've broken it up. Please consult the
following friendly articles:

  - [Frequently asked questions](doc/faq.md) ∾ but why...? ∾ does it...? ∾ can it...?
  
  - [How the FluxEngine works](doc/technical.md) ∾ nitty gritty of the
    sampler/sequencer hardware ∾ useful links on floppy drives ∾ why I'm not
    using an Arduino/STM32/ESP32/Raspberry Pi

  - [Making a FluxEngine](doc/building.md) ∾ what parts you need ∾ building it ∾
    setting up the toolchain ∾ compiling the firmware ∾ programming the board

  - [Using a FluxEngine](doc/using.md) ∾ what to do with your new hardware ∾
    flux files and image files ∾ knowing what you're doing

  - [Using GreaseWeazle hardware with the FluxEngine client
	software](doc/greaseweazle.md) ∾ what works ∾ what doesn't work ∾ where to
	go for help

  - [Troubleshooting dubious disks](doc/problems.md) ∾ it's not an exact
	science ∾ the sector map ∾ clock detection and the histogram

  - [Checking your drive](doc/driveresponse.md) ∾ you can't do that with that ∾
	measuring your drive's ability to work with exotic formats

Which?
------

The current support state is as follows.

Dinosaurs (🦖) have yet to be observed in real life --- I've written the
decoder based on Kryoflux (or other) dumps I've found. I don't (yet) have
real, physical disks in my hand to test the capture process.

Unicorns (🦄) are completely real --- this means that I've read actual,
physical disks with these formats and so know they work (or had reports from
people who've had it work).

### Old disk formats

| Format                                    | Read? | Write? | Notes |
|:------------------------------------------|:-----:|:------:|-------|
| [IBM PC compatible](doc/disk-ibm.md)      |  🦄   |   🦄   | and compatibles (like the Atari ST) |
| [Atari ST](doc/disk-atarist.md)           |  🦄   |   🦄   | technically the same as IBM, almost |
| [Acorn ADFS](doc/disk-acornadfs.md)       |  🦄   |   🦖*  | single- and double- sided           |
| [Acorn DFS](doc/disk-acorndfs.md)         |  🦄   |   🦖*  |                                     |
| [Ampro Little Board](doc/disk-ampro.md)   |  🦖   |   🦖*  |                                     |
| [Apple II DOS 3.3](doc/disk-apple2.md)    |  🦄   |   🦖   | doesn't do logical sector remapping |
| [Amiga](doc/disk-amiga.md)                |  🦄   |   🦄   |                                     |
| [Commodore 64 1541/1581](doc/disk-c64.md) |  🦄   |   🦄   | and probably the other formats      |
| [Brother 120kB](doc/disk-brother.md)      |  🦄   |   🦄   |                                     |
| [Brother 240kB](doc/disk-brother.md)      |  🦄   |   🦄   |                                     |
| [Brother FB-100](doc/disk-fb100.md)       |  🦖   |        | Tandy Model 100, Husky Hunter, knitting machines |
| [Macintosh 400kB/800kB](doc/disk-macintosh.md)  |  🦄   |   🦄   |                                     |
| [NEC PC-98](doc/disk-ibm.md)              |  🦄   |   🦄   | trimode drive not required          |
| [Sharp X68000](doc/disk-ibm.md)           |  🦄   |   🦄   |                                     |
| [TRS-80](doc/disk-trs80.md)               |  🦖   |   🦖*  | a minor variation of the IBM scheme |
{: .datatable }

`*`: these formats are variations of the generic IBM format, and since the
IBM writer is completely generic, it should be configurable for these
formats... theoretically. I don't have the hardware to try it.

### Even older disk formats

These formats are for particularly old, weird architectures, even by the
standards of floppy disks. They've largely been implemented from single flux
files with no access to physical hardware. Typically the reads were pretty
bad and I've had to make a number of guesses as to how things work. They do,
at least, check the CRC so what data's there is probably good.

| Format                                   | Read? | Write? | Notes |
|:-----------------------------------------|:-----:|:------:|-------|
| [AES Superplus / No Problem](doc/disk-aeslanier.md) |  🦖   | | hard sectors! |
| [Durango F85](doc/disk-durangof85.md)    |  🦖   |        | 5.25" |
| [DVK MX](doc/disk-mx.md)                 |  🦖   |        | Soviet PDP-11 clone |
| [VDS Eco1](doc/disk-eco1.md)             |  🦖   |        | 8" mixed format |
| [Micropolis](doc/disk-micropolis.md)     |  🦄   |        | Micropolis 100tpi drives |
| [Northstar](doc/disk-northstar.md)       |  🦖   |   🦖   | 5.25" hard sectors |
| [TI DS990 FD1000](doc/disk-tids990.md)   |  🦄   |  🦄    | 8" |
| [Victor 9000](doc/disk-victor9k.md)      |  🦖   |        | 5.25" GCR encoded |
| [Zilog MCZ](doc/disk-zilogmcz.md)        |  🦖   |        | 8" _and_ hard sectors |
{: .datatable }

### Notes

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
    implemented write support for PC disks because they're boring and I'm lazy,
    and also because they vary so much that figuring out how to specify them
    is hard.

If you have samples of weird disks, and want to send them to me --- either
FluxEngine, Kryoflux or Catweasel dumps, or (even better) actually physically
--- I can identify them and add support.

Please note that at this point I am *not interested in copy protected disks*.
It's not out of principle. It's just they'll drive me insane. FluxEngine will
most likely be able to read the data fine, unless they're doing bizarre
things like spiral tracks or partially encoded data, but let's stick with
normal conventionally formatted disks for the time being!

But!
----

That said, I need to post a warning.

<div style="text-align: center; color: red">
<b>********** BIG DISCLAIMERY WARNING WITH ASTERISKS **********</b>
</div>

Floppy disks are old, unreliable, and frequently damaged and/or filthy. I
expect you to know what you're doing and be responsible for your own actions.
It's entirely possible for a damaged disk, when read, to scrape the magnetic
coating off the disk and pack it into the drive's disk head, not only
permanently damaging the drive, but also irrecoverably destroying any data on
the disk.

If this happens and you complain to me, I will be sympathetic but
fundamentally unhelpful. Proceed at your own risk.

Remember: **FluxEngine is not a substitute for a real data recovery
service.** Is your data worth money to you? If so, don't try to read it using
an open source project hacked together by some person you've never met on the
internet.

Also, remember to clean your disk heads.

<div style="text-align: center; color: red">
<b>********** END OF WARNING **********</b>
</div>

Who?
----

The FluxEngine was designed, built and written by me, David Given. You may
contact me at dg@cowlark.com, or visit my website at http://www.cowlark.com.
There may or may not be anything interesting there.

License
-------

Everything here _except the contents of the `dep` directory_ is © 2019 David
Given and is licensed under the MIT open source license. Please see
[COPYING](COPYING) for the full text. The tl;dr is: you can do what you like
with it provided you don't claim you wrote it.

As an exception, `dep/fmt` contains a copy of [fmt](http://fmtlib.net),
maintained by Victor Zverovich (`vitaut <https://github.com/vitaut>`) and
Jonathan Müller (`foonathan <https://github.com/foonathan>`) with
contributions from many other people. It is licensed under the terms of the
BSD license. Please see the contents of the directory for the full text.

As an exception, `dep/emu` contains parts of the OpenBSD C library
code, maintained by Todd Miller and William A. Rowe (and probably others). It is licensed
under the terms of the 3-clause BSD license. Please see the contents of the
directory for the full text. It's been lightly modified by me.

As an exception, `dep/agg` contains parts of the Anti-Grain Antialiasing
library, written by Maxim Semanarev (and others). It is licensed under the
terms of the 3-clause BSD license. Please see the contents of the directory for
the full text. It's been lightly modified by me.

As an exception, `dep/stb` contains parts of the libstb utility library,
written by Sean T Barett (and others). It is public domain/Unlicense/MIT
licensed, at your choice. Please see the contents of the directory for the full
text.

As an exception, `dep/snowhouse` contains the snowhouse assertion library,
taken from https://github.com/banditcpp/snowhouse. It is Boost Standard License
1.0 licensed. Please see the contents of the directory for the full text.

As an exception, `dep/libusbp` contains the libusbp library, taken from
https://github.com/pololu/libusbp. It is MIT licensed. Please see the contents
of the directory for the full text.
