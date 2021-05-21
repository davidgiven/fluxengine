Disk: Acorn DFS
===============

Acorn DFS disks are pretty standard FM encoded IBM scheme disks, with
256-sectors and 0-based sector identifiers. There's nothing particularly
special here.

DFS disks are all single sided, but allow the other side of the disk to be
used as another drive. FluxEngine supports these; read one side at a time
with `--heads 0` or `--heads 1`.

DFS comes in two varieties, 40 track and 80 track. These should both work.  For
40 track you'll want `--cylinders 0-79x2`. Some rare disks are both at the same
time. FluxEngine can read these but it requires a bit of fiddling as they have
the same tracks on twice.

Reading discs
-------------

Just do:

```
fluxengine read acorndfs
```

You should end up with an `acorndfs.img` of the appropriate size for your disk
format. This is an alias for `fluxengine read ibm` with preconfigured
parameters.
