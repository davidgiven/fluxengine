Disk: Apple II
==============

Apple II disks are nominally fairly sensible 40-track, single-sided, 256
bytes-per-sector jobs. However, they come in two varieties: DOS 3.3/ProDOS and
above, and pre-DOS 3.3. They use different GCR encoding systems, dubbed
6-and-2 and 5-and-3, and are mutually incompatible (although in some rare
cases you can mix 6-and-2 and 5-and-3 sectors on the same disk).

The difference is in the drive controller; the 6-and-2 controller is capable
of a more efficient encoding, and can fit 16 sectors on a track, storing
140kB on a disk. The 5-and-3 controller can only fit 13, with a mere 114kB.

Both formats use GCR (in different varieties) in a nice, simple grid of
sectors, unlike the Macintosh. Like the Macintosh, there's a crazy encoding
scheme applied to the data before it goes down on disk to speed up
checksumming.

In addition, a lot of the behaviour of the drive was handled in software.
This means that Apple II disks can do all kinds of weird things, including
having spiral tracks! Copy protection for the Apple II was even madder than
on other systems.

FluxEngine can only read well-behaved 6-and-2 disks. It doesn't even try to
handle the weird stuff.

Apple DOS also applies logical sector remapping on top of the physical sector
numbering on the disk, and this _varies_ depending on what the disk is for.
FluxEngine can remap the sectors from physical to logical using modifiers.  If
you don't specify a remapping modifier, you get the sectors in the order they
appear on the disk.


Reading discs
-------------

Just do:

```
fluxengine read apple2
```

You should end up with an `apple2.img` which is 143360 bytes long.
It will be in physical sector ordering. You can specify a sector ordering,
`appledos` or `prodos` to get an image intended for use in an emulator,
due to the logical sector mapping issue described above:

```
fluxengine read apple2 prodos
```

Writing discs
-------------

Just do:
```
fluxengine write apple2 -i apple2.img
```

If your image is in logical sector ordering (images intended for emulators
usually are), specify a modifier of `appledos` or `prodos`:

```
fluxengine write apple2 prodos -i apple2.img
```


Useful references
-----------------

  - [Beneath Apple DOS](https://fabiensanglard.net/fd_proxy/prince_of_persia/Beneath%20Apple%20DOS.pdf)

  - [MAME's ap2_dsk.cpp file](https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib/formats/ap2_dsk.cpp)
