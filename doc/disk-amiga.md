Disk: Amiga
===========

Amiga disks use MFM, but don't use IBM scheme. Instead, the entire track is
read and written as a unit, with each sector butting up against the previous
one. This saves a lot of space which allows the Amiga to not just store 880kB
on a DD disk, but _also_ allows an extra 16 bytes of metadata per sector.

Bizarrely, the data in each sector is stored with all the odd bits first, and
then all the even bits. This is tied into the checksum algorithm, which is
distinctly subpar and not particularly good at detecting errors.

Reading discs
-------------

Just do:

```
.obj/fe-readamiga
```

You should end up with an `amiga.adf` which is 901120 bytes long (for a
normal DD disk) --- it ought to be a perfectly normal ADF file which you can
use in an emulator.

Useful references
-----------------

  - [The Amiga Floppy Boot Process and Physical
    Layout](https://wiki.amigaos.net/wiki/Amiga_Floppy_Boot_Process_and_Physical_Layout)

  - [The Amiga Disk File FAQ](http://lclevy.free.fr/adflib/adf_info.html)
