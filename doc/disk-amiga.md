Disk: Amiga
===========

Amiga disks use MFM, but don't use IBM scheme. Instead, the entire track is
read and written as a unit, with each sector butting up against the previous
one. This saves a lot of space which allows the Amiga to not just store 880kB
on a DD disk, but _also_ allows an extra 16 bytes of metadata per sector.

Bizarrely, the data in each sector is stored with all the odd bits first, and
then all the even bits. This is tied into the checksum algorithm, which is
distinctly subpar and not particularly good at detecting errors.

Reading disks
-------------

Just do:

```
fluxengine read amiga
```

You should end up with an `amiga.adf` which is 901120 bytes long (for a
normal DD disk) --- it ought to be a perfectly normal ADF file which you can
use in an emulator.

If you want the metadata as well, specify a 528 byte sector size for the
output image:

```
fluxengine read amiga -o amiga.adf:b=528
```

You will end up with a 929280 byte long image which you probably _can't_ use
in an emulator; each sector will contain the 512 bytes of user payload
followed by the 16 bytes of metadata.

Writing disks
-------------

Just do:

```
fluxengine write amiga -i amiga.adf
```

This will rake a normal 901120 byte long ADF file and write it to a DD disk.
Note that writing to an HD disk will probably not work (this will depend on
your drive and disk and potential FluxEngine bugs I'm still working on ---
please [get in touch](https://github.com/davidgiven/fluxengine/issues/new) if
you have any insight here).

If you want to write the metadata as well, specify a 528 byte sector size for
the output image and supply a 929280 byte long file as described above.

```
fluxengine write amiga -i amiga.adf:b=528
```

Useful references
-----------------

  - [The Amiga Floppy Boot Process and Physical
    Layout](https://wiki.amigaos.net/wiki/Amiga_Floppy_Boot_Process_and_Physical_Layout)

  - [The Amiga Disk File FAQ](http://lclevy.free.fr/adflib/adf_info.html)
