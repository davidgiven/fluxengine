Disk: Brother FB-100
====================

The Brother FB-100 is a serial-attached smart floppy drive used by a several
different machines for mass storage, including the Tandy Model 100 and
clones, the Husky Hunter 2, and (bizarrely) several knitting machines. It was
usually rebadged, sometimes with a cheap paper label stuck over the Brother
logo: the most common variant appears to be the Tandy Portable Disk Drive or
TPDD:

<a href="http://www.old-computers.com/museum/computer.asp?c=233&st=1"> <img src="tdpp.jpg" alt="A Tandy Portable Disk Drive"/></a>

It's a bit of an oddball: the disk encoding is FM with a very custom record
scheme: 40-track single-sided 3.5" disks storing 100kB or so each. Each track
had only _two_ sectors, each 1280 bytes, but with an additional 17 bytes of
ID data used for filesystem management.

There was also apparently a TPDD-2 which could store twice as much data, but
I don't have access to one of those disks.

Reading discs
-------------

Just do:

```
.obj/fe-readfb100
```

You should end up with an `fb11.img` of the appropriate size. It's a simple
array of 80 1297-byte sectors (17 bytes for the ID record plus 1280 bytes for
the data).