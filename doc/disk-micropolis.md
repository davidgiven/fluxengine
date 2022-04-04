Disk: Micropolis
================

Micropolis MetaFloppy disks use MFM and hard sectors. Mod I was 48 TPI and
stored 143k per side. Mod II was 100 TPI and stored 315k per side. Each of the
16 sectors contains 266 bytes of "user data," allowing 10 bytes of metadata for
use by the operating system. Micropolis DOS (MDOS) used the metadata bytes, but
CP/M did not.

Some later systems were Micropolis-compatible and so were also 100 TPI, like
the Vector Graphic Dual-Mode Disk Controller which was paired with a Tandon
drive.

**Important note:** You _cannot_ read these disks with a normal PC drive, as
these drives are 96tpi. The track spacing is determined by the physical geometry
of the drive and can't be changed in software. You'll need to get hold of a
100tpi Micropolis drive. Luckily these seem to use the same connector and
pinout as a 96tpi PC 5.25" drive. In use they should be identical.

Reading disks
-------------

Based on your floppy drive, just do one of:

```
fluxengine read micropolis143 # single-sided Mod I
fluxengine read micropolis287 # double-sided Mod I
fluxengine read micropolis315 # single-sided Mod II
fluxengine read micropolis630 # double-sided Mod II
```

You should end up with a `micropolis.img` of the corresponding size. The image
is written in CHS order, but HCS is generally used by CP/M tools so
double-sided disk images may need to be post-processed. Half-full double-sided
disks can be read as single-sided disks to work around the problem.

The [CP/M BIOS](https://www.seasip.info/Cpm/bios.html) defined SELDSK, SETTRK,
and SETSEC, but no function to select the head/side. Double-sided floppies
could be represented as having either twice the number of sectors, for CHS, or
twice the number of tracks, HCS; the second side's tracks in opposite order
logically followed the first side (e.g., tracks 77-153). Micropolis disks
tended to be the latter.

It's also possible to output to VGI, which retains OS-specific "user data" and
machine-specific ECC. Add "vgi" to the command line after the chosen Micropolis
profile:
```
fluxengine read micropolis143 vgi # single-sided Mod I
fluxengine read micropolis287 vgi # double-sided Mod I
fluxengine read micropolis315 vgi # single-sided Mod II
fluxengine read micropolis630 vgi # double-sided Mod II
```

You should end up with a `micropolis.vgi` instead. The format is well-defined
for double-sided disks so post-processing is not necessary.

While most operating systems use the standard Micropolis checksum, Vector
Graphic MZOS uses a unique checksum.  The decoder will automatically detect
the checksum type in use; however, a specific checksum type may be forced
using the `--decoder.micropolis.checksum_type=n` where the type is one of:

| Type | Description                             |
|------|-----------------------------------------|
| 0    | Automatically detect                    |
| 1    | Standard Micropolis (MDOS, CP/M, OASIS) |
| 2    | Vector Graphic MZOS                     |


Writing disks
-------------

Just do one of:

```
fluxengine write micropolis143 # single-sided Mod I
fluxengine write micropolis287 # double-sided Mod I
fluxengine write micropolis315 # single-sided Mod II
fluxengine write micropolis630 # double-sided Mod II

fluxengine write micropolis143 vgi # single-sided Mod I
fluxengine write micropolis287 vgi # double-sided Mod I
fluxengine write micropolis315 vgi # single-sided Mod II
fluxengine write micropolis630 vgi # double-sided Mod II
```

Useful references
-----------------

  - [Micropolis 1040/1050 S-100 Floppy Disk Subsystems User's Manual][micropolis1040/1050].
    Section 6, pages 261-266. Documents pre-ECC sector format. Note that the
    entire record, starting at the sync byte, is controlled by software

  - [Vector Graphic Dual-Mode Disk Controller Board Engineering Documentation][vectordualmode].
    Section 1.6.2. Documents ECC sector format

  - [AltairZ80 Simulator Usage Manual][altairz80]. Section 10.6. Documents ECC
    sector format and VGI file format

[micropolis1040/1050]: http://www.bitsavers.org/pdf/micropolis/metafloppy/1084-01_1040_1050_Users_Manual_Apr79.pdf
[vectordualmode]: http://bitsavers.org/pdf/vectorGraphic/hardware/7200-1200-02-1_Dual-Mode_Disk_Controller_Board_Engineering_Documentation_Feb81.pdf
[altairz80]: http://www.bitsavers.org/simh.trailing-edge.com_201206/pdf/altairz80_doc.pdf
