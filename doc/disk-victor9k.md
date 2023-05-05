Disk: Victor 9000
=================

The Victor 9000 / Sirius One was a rather strange old 8086-based machine
which used a disk format very reminiscent of the Commodore format; not a
coincidence, as Chuck Peddle designed them both. They're 80-track, 512-byte
sector GCR disks, with a variable-speed drive and a varying number of sectors
per track --- from 19 to 12. Disks can be double-sided, meaning that they can
store 1224kB per disk, which was almost unheard of back then. Because the way
that the tracks on head 1 are offset from head 0 (this happens with all disks),
the speed zone allocation on head 1 differs from head 0...

| Zone | Head 0 tracks | Head 1 tracks | Sectors | Original period (ms) |
|:----:|:-------------:|:-------------:|:-------:|:--------------------:|
| 0    | 0-3           |               | 19      | 237.9                |
| 1    | 4-15          | 0-7           | 18      | 224.5                |
| 2    | 16-26         | 8-18          | 17      | 212.2                |
| 3    | 27-37         | 19-29         | 16      | 199.9                |
| 4    | 38-47\*       | 30-39\*       | 15      | 187.6                |
| 5    | 48-59         | 40-51         | 14      | 175.3                |
| 6    | 60-70         | 52-62         | 13      | 163.0                |
| 7    | 71-79         | 63-74         | 12      | 149.6                |
| 8    |               | 75-79         | 11      | 144.0                |

(The Original Period column is the original rotation rate. When used in
FluxEngine, the disk always spins at 360 rpm, which corresponds to a rotational
period of 166 ms.)

\*The Victor 9000 Hardware Reference Manual has a bug in the documentation 
and lists Zone 4 as ending with track 48 on head 0 and track 40 on head 1. 
The above table matches observed data on various disks and the assembly 
code in the boot loader, which ends Zone 4 with track 47 on head 0 
and track 39 on Head 1.

FluxEngine can read and write both the single-sided and double-sided variants. 

Reading discs
-------------

Just do:

```
fluxengine read victor9k <format>

```

...where `<format>` can be `--612` for a single-sided disk or `--1224` for a
double-sided disk. 

Writing discs
-------------

Just do:

```
fluxengine write victor9k <format> -i victor9k.img
```

`<format>` is as above.

Useful references
-----------------

  - [The Victor 9000 technical reference manual](http://bitsavers.org/pdf/victor/victor9000/Victor9000TechRef_Jun82.pdf)

  - [DiskFerret's Victor 9000 format guide](https://discferret.com/wiki/Victor_9000_format)

