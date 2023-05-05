Disk: Northstar
================

Northstar Floppy disks use 10-sector hard sectored disks with either FM or MFM
encoding.  They may be single- or double-sided.  Each of the 10 sectors contains
256 (FM) or 512 (MFM) bytes of data.  The disk has 35 cylinders, with tracks 0-
34 on side 0, and tracks 35-69 on side 1.  Tracks on side 1 are numbered "back-
wards" in that track 35 corresponds to cylinder 34, side 1, and track 69
corresponds to cylinder 0, side 1.

The Northstar sector format does not include any head positioning information.
As such, reads from Northstar floppies need to by synchronized with the index
pulse, in order to properly identify the sector being read.  This is handled
automatically by FluxEngine.

Due to the nature of the track ordering on side 1, an .nsi image reader and
writer are provided for double-sided disks.  The .nsi image writer supports
both single- and double-sided disks; however single-sided .nsi images are
equivalent to .img images.

Reading disks
-------------

To read a double-sided North Star floppy, run:

```
fluxengine read northstar <format>
```

...where `<format>` is one of the formats listed below.

You should end up with a `northstar.nsi` with a file size dependent on the floppy
disk type.

Writing disks
-------------

To write a double-sided North Star floppy, run:

```
fluxengine write northstar <format> -i image_to_write.nsi
```

...where `<format>` is one of the formats listed below.

Available formats
-----------------

The following formats are supported:

| Format name    | Disk Type                           | File Size (bytes) |
| -------------- | ----------------------------------- | ----------------- |
| `--87`         | Single-Sided, Single-Density (SSSD) | 89,600            |
| `--175`        | Single-Sided, Double-Density (SSDD) | 179,200           |
| `--350`        | Double-Sided, Double-Density (DSDD) | 358,400           |

Useful references
-----------------

  - [MICRO-DISK SYSTEM MDS-A-D DOUBLE DENSITY Manual][northstar_mds].
    Page 33 documents sector format for single- and double-density.

[northstar_mds]: http://bitsavers.org/pdf/northstar/boards/Northstar_MDS-A-D_1978.pdf
