Disk: Atari ST
==============

Atari ST disks are standard MFM encoded IBM scheme disks without an IAM header.
Disks are typically formatted 512 bytes per sector with between 9-10 (sometimes
11!) sectors per track and 80-82 tracks per side.


Reading disks
-------------

Just do:

    fluxengine read atarist

...and you'll end up with an `atarist.st` file. The size of the disk image will
vary depending on the format.

Writing disks
-------------

FluxEngine can also write Atari ST scheme disks.

The syntax is:

    fluxengine write atarist -i input.st <options>

The format of `input.st` will vary depending on the kind of disk you're writing,
which is configured by the options. By default FluxEngine will write an 80
track, 9 sector, double-sided disk. If that doesn't match your target format you
will need to pass some options. There are some presets, which you will almost
certainly want to use if possible:

  - `--st-preset-360`: a 360kB 3.5" disk, with 80 cylinders,
  1 side, and 9 sectors per track.
  - `--st-preset-370`: a 370kB 3.5" disk, with 82 cylinders,
  1 side, and 9 sectors per track.
  - `--st-preset-400`: a 400kB 3.5" disk, with 80 cylinders,
  1 side, and 10 sectors per track.
  - `--st-preset-410`: a 410kB 3.5" disk, with 82 cylinders,
  1 side, and 10 sectors per track.
  - `--st-preset-720`: a 720kB 3.5" disk, with 80 cylinders,
  2 sides, and 9 sectors per track.
  - `--st-preset-740`: a 740kB 3.5" disk, with 82 cylinders,
  2 sides, and 9 sectors per track.
  - `--st-preset-800`: a 800kB 3.5" disk, with 80 cylinders,
  2 sides, and 10 sectors per track.
  - `--st-preset-820`: a 820kB 3.5" disk, with 82 cylinders,
  2 sides, and 10 sectors per track.

These options simply preset the output destination flag (`-d`) and the
following, lower-level options. Note that options are processed left to right,
so it's possible to use a preset and then change some settings. To see the
values for a preset, simply append `--help`.
  - `--st-sector-size=N`: the size of a sector, in bytes. Must be a power of
  two.
  - `--st-gap1-bytes=N`: the size of gap 1 in bytes (between the IAM record
  and the first sector record).
  - `--st-gap2-bytes=N`: the size of gap 2 in bytes (between each sector
  record and the data record).
  - `--st-gap3-bytes=N`: the size of gap 3 in bytes (between the data record
  and the next sector record).
  - `--st-sector-skew=0123...`: a string representing the order in which to
  write sectors: each character represents on sector, with `0` being the
  first. Sectors 10 and above are represented as letters from `A` up.


Useful references
-----------------

  - [Atari ST Floppy Drive Hardware Information](https://info-coach.fr/atari/hardware/FD-Hard.php) by Jean Louis-Guerin

  - [Atari ST Floppy Drive Software Information](https://info-coach.fr/atari/software/FD-Soft.php) by Jean Louis-Guerin
