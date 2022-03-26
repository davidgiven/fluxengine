Disk: Agat
==========

The Agat (Russian: Агат) was a series Soviet-era computer, first released about
1983. These were based around a 6502 and were nominally Apple II-compatible
although with enough differences to be problematic.

They could use either standard Apple II 140kB disks, or a proprietary 840kb
MFM-based double-sided format. FluxEngine supports both of these; this profile
is for the proprietary format. for the Apple II format, use the [Apple II
profile](disk-apple2.md).



Reading discs
-------------

Just do:

```
fluxengine read agat840
```

You should end up with an `agat840.img` which is 860160 bytes long.


Useful references
-----------------

  - [Magazine article on the
	Agat](https://sudonull.com/post/54185-Is-AGAT-a-bad-copy-of-Apple)

  - [Forum thread with (some) documentation on the
	format](https://torlus.com/floppy/forum/viewtopic.php?t=1385)
