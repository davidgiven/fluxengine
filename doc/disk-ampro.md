Disk: Ampro Little Board
========================

The Ampro Little Board was a very simple and cheap Z80-based computer from
1984, which ran CP/M. It was, in fact, a single PCB which you could mount
on the bottom of a 5.25" drive.

[All about the Ampro Little Board](http://oldcomputers.net/ampro-little-board.html)

It stored either 400kB on a double-sided 40-track drive or 800kB on a
double-sided 80 track drive. The disk format it used was a slightly quirky
variation of the standard MFM IBM scheme --- sector numbering starts at 17
rather than 1 (or Acorn's 0). FluxEngine supports this.


Reading discs
-------------

Just do:

```
fluxengine read ampro
```

You should end up with an `ampro.img` which is 409600 or 819200 bytes long.
You can pass this straight into [cpmtools](http://www.moria.de/~michael/cpmtools/):

```
$ cpmls -f ampdsdd ampro.img
0:
-a60014.e
amprodsk.com
bitchk.doc
bitchk.mac
cpmmac.mac
dir.com
himem.doc
himem.mac
kaydiag.lbr
kayinfo.lbr
...etc...
```

Useful references
-----------------

  - [The Ampro Little Board](http://oldcomputers.net/ampro-little-board.html)
