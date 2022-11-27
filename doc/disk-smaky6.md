Disk: Smaky 6
=============

The Smaky 6 is a Swiss computer from 1978 produced by Epsitec. It's based
around a Z80 processor and has one or two Micropolis 5.25" drives which use
16-sector hard sectored disks. The disk format is single-sided with 77 tracks
and 256-byte sectors, resulting in 308kB disks. It uses MFM with a custom
sector record scheme. It was later superceded by a 68000-based Smaky which used
different disks.

FluxEngine supports these, although because the Micropolis drives use a 100tpi
track pitch, you can't read Smaky 6 disks with a normal PC 96tpi drive. You
will have to find a 100tpi drive from somewhere (they're rare).

Reading disks
-------------

You must use a 100-tpi 80-track 5.25" floppy drive.

To read a Smaky 6 floppy, do:

```
fluxengine read smaky6
```

You should end up with a `smaky6.img` file.


Filesystem access
-----------------

There is experimental read-only support for the Smaky 6 filesystem, allowing
the directory to be listed and files read from disks. It's not known whether
this is completely correct, so don't trust it! See the [Filesystem
Access](filesystem.md) page for more information.


Useful references
-----------------

  - [Smaky Info, 1978-2002 (in French)](https://www.smaky.ch/theme.php?id=sminfo)
    

