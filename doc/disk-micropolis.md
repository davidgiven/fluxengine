Disk: Micropolis
================

Micropolis MetaFloppy disks use MFM and hard sectors. They were 100 TPI and
stored 315k per side. Each of the 16 sectors contains 266 bytes of "user data,"
allowing 10 bytes of metadata for use by the operating system. Micropolis DOS
(MDOS) used the metadata bytes, but CP/M did not.

Some later systems were Micropolis-compatible and so were also 100 TPI, like
the Vector Graphic Dual-Mode Disk Controller which was paired with a Tandon
drive.

Reading disks
-------------

Just do:

```
fluxengine read micropolis
```

You should end up with a `micropolis.img` which is 630784 bytes long (for a
normal DD disk). The image is written in CHS order, but HCS is generally used
by CP/M tools so the image needs to be post-processed. For only half-full disks
or single-sided disks, you can use `-s :s=0` to read only one side of the disk
which works around the problem.

The [CP/M BIOS](https://www.seasip.info/Cpm/bios.html) defined SELDSK, SETTRK,
and SETSEC, but no function to select the head/side. This caused dual-sided
floppies to be represented as twice the number of tracks with the second side's
tracks logically following the first side (e.g., tracks 77-153). That produces
results mostly indistinguishable from HCS.

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
