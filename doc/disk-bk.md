Disk: Elektronica BK
====================

The BK (an abbreviation for бытовой компьютер --- 'home computer' in Russian)
is a Soviet era personal computer from Elektronika based on a PDP-11
single-chip processor. It was the _only_ official, government approved home
computer in mass production at the time.

It got a floppy interface in 1989 when the 128kB BK-0011 was released. This
used a relatively normal double-sided IBM scheme format with 80 sectors and ten
sectors per track, resulting in 800kB disks. The format is, in fact, identical
to the Atari ST 800kB format. Either 5.25" or 3.5" drives were used depending
on what was available at the time, with the same format on both.

Reading disks
-------------

Just do:

```
fluxengine read bk800 -o bk800.img
```

You should end up with an `bk800.img` containing all the sectors concatenated
one after the other in CHS order. This will work on both 5.25" and 3.5" drives.

Writing disks
-------------

Just do:

```
fluxengine write bk800 -i bk800.img
```

This will write the disk image to either a 5.25" or 3.5" drive.

