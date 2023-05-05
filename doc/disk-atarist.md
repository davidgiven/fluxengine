Disk: Atari ST
==============

Atari ST disks are standard MFM encoded IBM scheme disks without an IAM header.
Disks are typically formatted 512 bytes per sector with between 9-10 (sometimes
11!) sectors per track and 80-82 tracks per side.

For some reason, occasionally formatting software will put an extra IDAM record
with a sector number of 66 on a disk, which can horribly confuse things. The
Atari profiles below are configured to ignore these.

Be aware that many PC drives (including mine) won't do the 82 track formats. 

Reading disks
-------------

Just do:

    fluxengine read <format>

...and you'll end up with an `atarist.st` file. The size of the disk image will
vary depending on the format. This is an alias for `fluxengine read ibm` with
preconfigured parameters.

Note that the profile by default assumes a double-sided disk; if you're reading
a single-sided disk, add `--heads 0` to prevent FluxEngine from looking at the
other side and getting confused by any data it sees there.

Writing disks
-------------

FluxEngine can also write Atari ST scheme disks.

The syntax is:

    fluxengine write atarist -i input.st <format>

Available formats
-----------------

`<format>` can be one of these:

  - `--360`: a 360kB 3.5" disk, with 80 cylinders, 1 side, and 9 sectors per
    track.
  - `--370`: a 370kB 3.5" disk, with 82 cylinders, 1 side, and 9 sectors per
    track.
  - `--400`: a 400kB 3.5" disk, with 80 cylinders, 1 side, and 10 sectors per
    track.
  - `--410`: a 410kB 3.5" disk, with 82 cylinders, 1 side, and 10 sectors per
    track.
  - `--720`: a 720kB 3.5" disk, with 80 cylinders, 2 sides, and 9 sectors per
    track.
  - `--740`: a 740kB 3.5" disk, with 82 cylinders, 2 sides, and 9 sectors per
    track.
  - `--800`: a 800kB 3.5" disk, with 80 cylinders, 2 sides, and 10 sectors per
    track.
  - `--820`: a 820kB 3.5" disk, with 82 cylinders, 2 sides, and 10 sectors per
    track.

See [the IBM format documentation](disk-ibm.md) for more information. Note that
only some PC 3.5" floppy disk drives are capable of seeking to the 82nd track.


Useful references
-----------------

  - [Atari ST Floppy Drive Hardware
	Information](https://info-coach.fr/atari/hardware/FD-Hard.php) by Jean
	Louis-Guerin

  - [Atari ST Floppy Drive Software
	Information](https://info-coach.fr/atari/software/FD-Soft.php) by Jean
	Louis-Guerin
