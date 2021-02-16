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
vary depending on the format. This is an alias for `fluxengine read ibm` with
preconfigured parameters.

Writing disks
-------------

FluxEngine can also write Atari ST scheme disks.

The syntax is:

    fluxengine write ibm -i input.st <options>

The format of `input.st` will vary depending on the kind of disk you're
writing, which is configured by the options.  You will almost certainly need to
pass one of these:

  - `--ibm-preset-atarist-360`: a 360kB 3.5" disk, with 80 cylinders, 1 side,
	and 9 sectors per track.
  - `--ibm-preset-atarist-370`: a 370kB 3.5" disk, with 82 cylinders, 1 side,
	and 9 sectors per track.
  - `--ibm-preset-atarist-400`: a 400kB 3.5" disk, with 80 cylinders, 1 side,
	and 10 sectors per track.
  - `--ibm-preset-atarist-410`: a 410kB 3.5" disk, with 82 cylinders, 1 side,
	and 10 sectors per track.
  - `--ibm-preset-atarist-720`: a 720kB 3.5" disk, with 80 cylinders, 2 sides,
	and 9 sectors per track.
  - `--ibm-preset-atarist-740`: a 740kB 3.5" disk, with 82 cylinders, 2 sides,
	and 9 sectors per track.
  - `--ibm-preset-atarist-800`: a 800kB 3.5" disk, with 80 cylinders, 2 sides,
	and 10 sectors per track.
  - `--ibm-preset-atarist-820`: a 820kB 3.5" disk, with 82 cylinders, 2 sides,
	and 10 sectors per track.

See [the IBM format documentation](disk-ibm.md) for more information.


Useful references
-----------------

  - [Atari ST Floppy Drive Hardware
	Information](https://info-coach.fr/atari/hardware/FD-Hard.php) by Jean
	Louis-Guerin

  - [Atari ST Floppy Drive Software
	Information](https://info-coach.fr/atari/software/FD-Soft.php) by Jean
	Louis-Guerin
