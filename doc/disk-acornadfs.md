Disk: Acorn ADFS
================

Acorn ADFS disks are pretty standard MFM encoded IBM scheme disks, although
with different sector sizes and with the 0-based sector identifiers rather
than 1-based sector identifiers. The index hole is ignored and sectors are
written whereever, requiring FluxEngine to do two revolutions to read a
disk.

There are various different kinds, which should all work out of the box.
Tested ones are:

  - ADFS L: 80 track, 16 sector, 2 sides, 256 bytes per sector == 640kB.
  - ADFE D/E: 80 track, 5 sector, 2 sides, 1024 bytes per sector == 800kB.
  - ADFS F: 80 track, 10 sector, 2 sides, 1024 bytes per sector == 1600kB.

I expect the others to work, but haven't tried them; [get in
touch](https://github.com/davidgiven/fluxengine/issues/new) if you have any
news. For ADFS S (single sided 40 track) you'll want `-s :s=0:t=0-79x2`. For
ADFS M (single sided 80 track) you'll want `-s :s=0`.

Be aware that Acorn logical block numbering goes all the way up side 0 and
then all the way up side 1. However, FluxEngine uses traditional disk images
with alternating sides, with the blocks from track 0 side 0 then track 0 side
1 then track 1 side 0 etc. Most Acorn emulators will use both formats, but
they might require nudging as the side order can't be reliably autodetected.

Reading discs
-------------

Just do:

```
fluxengine read adfs
```

You should end up with an `adfs.img` of the appropriate size for your disk
format.
