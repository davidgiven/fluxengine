Disk: Victor 9000
=================

The Victor 9000 / Sirius One was a rather strange old 8086-based machine
which used a disk format very reminiscent of the Commodore format; not a
coincidence, as Chuck Peddle designed them both. They're 80-track, 512-byte
sector GCR disks, with a variable-speed drive and a varying number of sectors
per track --- from 19 to 12. Disks can be double-sided, meaning that they can
store 1224kB per disk, which was almost unheard of back then. Because the way
that the tracks on head 1 are offset from head 0 (this happens with all disks),
the speed zone allocation on head 1 differ from head 0...

| Zone | Head 0 tracks | Head 1 tracks | Sectors | Original period (ms) |
|:----:|:-------------:|:-------------:|:-------:|:--------------------:|
| 0    | 0-3           |               | 19      | 237.9                |
| 1    | 4-15          | 0-7           | 18      | 224.5                |
| 2    | 16-26         | 8-18          | 17      | 212.2                |
| 3    | 27-37         | 19-29         | 16      | 199.9                |
| 4    | 38-48         | 30-40         | 15      | 187.6                |
| 5    | 49-59         | 41-51         | 14      | 175.3                |
| 6    | 60-70         | 52-62         | 13      | 163.0                |
| 7    | 71-79         | 63-74         | 12      | 149.6                |
| 8    |               | 75-79         | 11      | 144.0                |

(The Original Period column is the original rotation rate. When used in
FluxEngine, the disk always spins at 360 rpm, which corresponds to a rotational
period of 166 ms.)

FluxEngine can read and write the single-sided variant of these. (Double-sided
will be trivial to do, it's just not done yet.)

Reading discs
-------------

Just do:

```
fluxengine read victor9k-ss
```

You should end up with an `victor9k.img` which is 627200 bytes long.

**Big warning!** The image is triangular, where each track occupies a different
amount of space. Victor disk images are complicated due to the way the tracks
are different sizes and the odd sector size.

Writing discs
-------------

Just do:

```
fluxengine read victor9k-ss -i victor9k.img
```

**Big warning!** This uses the same triangular disk image that reading uses.


Useful references
-----------------

  - [The Victor 9000 technical reference manual](http://bitsavers.org/pdf/victor/victor9000/Victor9000TechRef_Jun82.pdf)

  - [DiskFerret's Victor 9000 format guide](https://discferret.com/wiki/Victor_9000_format)

