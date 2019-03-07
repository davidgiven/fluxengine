Frequently asked questions
==========================

**Q.** Why not just use a USB floppy drive? There are lots and they're cheap.

**A.** Because USB floppy drives typically support a very limited set of
formats --- typically only IBM 1440kB and 720kB. The FluxEngine should work
on (almost) anything, including the ones that IBM machines won't touch. Also,
as it's USB, it'll work happily on machines that were never designed for
floppy disks (like my development Chromebook).

**Q.** But aren't floppy disks obsolete?

**A.** Absolutely they are. That doesn't mean they've gone away. Good luck
with any old hardware, for example; a classic Mac won't boot without a
classic Mac boot disk, and you can't make them on PCs (because they're
weird). This is where the FluxEngine comes in.

**Q.** But how can you read and write non-PC formats on a PC floppy drive?

**A.** Because the FluxEngine hardware simply streams the raw magnetic flux
pulsetrain from the drive to the PC, and then the analysis is done off-line
in software. It doesn't rely on any floppy disk controller to interpret the
pulsetrain, so we can be a lot cleverer. In fact, the disk doesn't even have
to be spinning at the same speed.

**Q.** Does it work on 5.25" drives?

**A.** Yes! Although PC 5.25" drives spin at 360 RPM rather than 300 RPM,
which means there's only 166ms of data on one per track rather than 200ms;
if you try to write a 3.5" format disk onto one it probably won't work.

**Q.** Does it work on 8" drives?

**A.** Probably? You'd need an adapter to let you connect the drive to the
FluxEngine --- [you can get them](http://www.dbit.com/fdadap.html). I don't
have either the adapter, the drive, or any 8" disks. If anyone wants to give
it a try, please [tell me about
it](https://github.com/davidgiven/fluxengine/issues/new).

**Q.** Is this like KryoFlux / Catweasel / DiskFerret? Do you support KryoFlux stream files?

**A.** It's very like all of these; the idea's old, and lots of people have
*tried it (you can get away with any sufficiently fast microcontroller and
*enough RAM). FluxEngine can
read from KryoFlux stream files natively, and there's a tool which will let
you convert at least one kind of Catweasel file to FluxEngine's native flux
file format.

**Q.** Can I use this to make exact copies of disks?

**A.** No. FluxEngine can read disks, and it can write disks, but it can't
write flux files that it read. There's several reasons for this, including
but not limited to: amplification of disk noise causing unreadable disks;
needing to rearrange the read data so it all fits exactly between the index
markers; reproduction of non-repeatable noise (this was a common trick with
Apple II copy protection); etc. What FluxEngine prefers is to read a disk,
turn it into a filesystem image, and then synthesise flux from the filesystem
image and write _that_ to another disk.
