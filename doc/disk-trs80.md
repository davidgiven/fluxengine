Disk: TRS-80
============

The TRS-80 models I, III and IV (but not the II, 100, 2000, Colour Computer
or Pocket Computer) was a popular line of Z80-based home computers made by
Tandy Corporation and sold by Radio Shack. There were some of the first
generation of domestic micromputers, with the Model I released in 1978.

There were a myriad of different floppy disk interfaces, some produced by
Tandy and some by third parties, using all the various combinations of 40-
and 80-track, FM, MFM, etc.

Luckily the encoding scheme was mostly compatible with the IBM scheme, with a
few minor variations: when using FM encoding, the TRS-80 wrote the sectors on
track 17 (where the directory was) with a non-standard DAM byte.

FluxEngine's IBM reader can handle TRS-80 disks natively.

Reading discs
-------------

Just do:

```
fluxengine read ibm
```

You should end up with an `ibm.img` of the appropriate size. It's a simple
array of sectors in JV1 format.

If you've got a 40-track disk, use `-s :t=0-79x2`.

If you've got a single density disk, use `--read-fm=true`. (Double density is
the default.)
