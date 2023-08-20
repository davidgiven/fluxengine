Configuring for your drive
==========================

By default, the FluxEngine client assumes you have a PC 80 track double sided
high density drive, either 3.5" or 5.25", as these are the most common. This
may not be the case, and you need to tell the FluxEngine client what kind of
drive you have.

Forty track formats on an eighty track drive
--------------------------------------------

Forty-track drives have the same geometry as eighty-track drives, but have a
head that is twice as big, so halving the number of tracks on the disk.
Examples of forty track drives include the Commodore 1541, IBM 360kB, or the
Brother 120kB format (which uses a rare 3.5" single-sided forty-track drive).

When a forty-track disk is inserted into an eighty-track drive, then each head
position will only see _half_ of each track. For reading this isn't a problem
--- FluxEngine will actually read both halves and combine the results --- but
writing is more problematic. Traditionally, if you wanted to write a
forty-track disk in an eighty-track drive, you had to use a brand new disk; the
drive would write to one half of the track, leaving the other half blank. If
both halves contained data, then the wider head on a forty track drive would
pick both up, producing a garbled result. This led to a very confusing
situation where forty-track disks written on an eighty-track drive would read
and write fine on an eight-track drive but wouldn't work at all on a
forty-track drive.

FluxEngine is capable of both reading and writing forty-track formats on an
eighty-track drive. It avoids the situation described above by writing one half
of the track and then magnetically erasing the other half. This does produce a
weaker signal on the disk, but in my testing the disks work just fine in
forty-track drives.

Forty track formats on a forty track drive
------------------------------------------

If you actually have a forty track drive, you need to tell FluxEngine. This is
done by adding the special profile `40track_drive`:

```
fluxengine write ibm --360 40track_drive -i image.img -d drive:0
```

It should then Just Work. This is supported by both FluxEngine and Greaseweazle
hardware.

Obviously you can't write an eighty-track format using a forty-track drive!

Apple II drives
---------------

The Apple II had special drives which supported microstepping: when commanded
to move the head, then instead of moving in single-track steps as is done in
most other drives, the Apple II drive would move in quarter-track steps. This
allowed much less precise head alignment, as small errors could be corrected in
software. (The Brother word processor drives were similar.) The bus interface
is different from normal PC drives.

The FluxEngine client supports these with the `apple2_drive` profile:

```
fluxengine write apple2 apple2_drive -i image.img -d drive:0
```

This is supported only by Greaseweazle hardware.

Shugart drives
--------------

PC drives have a standard interface which doesn't really have a name but is
commonly referred to as 'the PC 34-pin interface'. There are a few other
interfaces, most notably the Shugart standard. This is also 34 pin and is very
similar to the PC interface but isn't quite electrically compatible. It
supports up to four drives on a bus, unlike the PC interface's two drives, but
the drives must be jumpered to configure them. This was mostly used by older
3.5" drives, such as those on the Atari ST. [the How It Works
page](technical.md) for the pinout.

The FluxEngine client supports these with the `shugart_drive` profile:

```
fluxengine write atarist720 shugart_drive -i image.img -d drive:0
```

(If you have a 40-track Shugart drive, use _both_ `shugart_drive` and
`40track_drive`.)

This is supported only by Greaseweazle hardware.

