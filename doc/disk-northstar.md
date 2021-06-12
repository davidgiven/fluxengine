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

You must use a 48-TPI (40-track) 300RPM 5.25" floppy drive.

To read a double-sided North Star floppy, run:

```
fluxengine read northstar
```

To read a single-sided North Star floppy, run:

```
fluxengine read <format> -heads 0
```

...where `<format>` is `northstar87`, `northstar175` or `northstar350`
depending on the format you want to read.

You should end up with a `northstar.nsi` with a file size dependent on the floppy
disk type:

| Disk Type                           | File Size (bytes) |
| ----------------------------------- | ------- |
| Single-Sided, Single-Density (SSSD) | 89,600  |
| Single-Sided, Double-Density (SSDD) | 179,200 |
| Double-Sided, Double-Density (DSDD) | 358,400 |

Writing disks
-------------

You must use a 48-TPI (40-track) 300RPM 5.25" floppy drive and make
sure that the drive's spindle speed is adjusted to exactly 300RPM.

To write a double-sided North Star floppy, run:

```
fluxengine write <format> -i image_to_write.nsi
```

...where `<format>` is `northstar87`, `northstar175` or `northstar350`
depending on the format you want to write.

Useful references
-----------------

  - [MICRO-DISK SYSTEM MDS-A-D DOUBLE DENSITY Manual][northstar_mds].
    Page 33 documents sector format for single- and double-density.

[northstar_mds]: http://bitsavers.org/pdf/northstar/boards/Northstar_MDS-A-D_1978.pdf
