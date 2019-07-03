My disk won't read properly!
============================

So you're trying to read a disk and it's not working. Well, you're not the
only one. Floppy disks are unreliable and awkward things. FluxEngine can
help, but the tools aren't very user friendly.

The good news is that as of 2019-04-30 the FluxEngine decoder algorithm has
been written to be largely fire-and-forget and is mostly self-adjusting.
However, there are still some things that can be tuned to produce better
reads.

The sector map
--------------

Every time FluxEngine does a read, it produces a dump like this:

```
H.SS Tracks --->
0. 0 XXXBXX...X.XXXX......?...........X...X.........X................................
0. 1 ..X..X.X..XB.B.B........X...........X.......X...................................
0. 2 X.XXXX.XX....XB.................X.X..X..X......X................................
0. 3 X.X..XXXX..?XXXX..................XX.X..........................................
0. 4 X.X..X....X.X.XX....?....?........XXXX..X.....X.................................
0. 5 XXXX...?..X.XBX...?......C......C?.X.X...?....X..........X......................
0. 6 XXXB.XX.XX???XXX...............CX.XXXX........X.................................
0. 7 XX..XX.XC..?.X......B.......X...X..XX...C.......................................
0. 8 X?.B...XXX.?..XX........X........XCXXX..X..X..X.................................
0. 9 BB.XX.X.X.X...BX.........C.......XXX...........X.....X..........................
0.10 BX.XX.XX.X..XX.B...X.............XXX........................................C...
0.11 .C.X.C..BXXBXBX?X................XX..X......X...................................
0.12 BX.XXX....BX..X......C....X......XXX.......XX..........................XXXXXXXXX
0.13 X..BXX..X?.XX.X....X..............XXXX.X....X...............XXXXXXXXXXXXXXXXXXXX
0.14 X...XXB..X.X..X....X...X..C........X?...........XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.15 X.BX.XX.X.XXX.X...........X.....X..X..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.16 XBXX...XX.X.X.XX........B..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.17 XXB..X.B....XX..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.18 XXCXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Good sectors: 978/1520 (64%)
Missing sectors: 503/1520 (33%)
Bad sectors: 39/1520 (2%)
80 tracks, 1 heads, 19 sectors, 512 bytes per sector, 760 kB total
```

This is the **sector map**, and is showing me the status of every sector it
found on the disk. (Tracks on the X-axis, sectors on the Y-axis.) This is a
very bad read from a [Victor 9000](disk-victor9k.md) disk; good reads
shouldn't look like this. A dot represents a good sector. A B is one where
the CRC check failed; an X is one which couldn't be found at all; a ? is a
sector which was found but contained no data.

At the very bottom there's a summary: 64% good sectors. Let me try and improve
that.

(You may notice the wedge of Xs in the bottom right. This is because the
Victor 9000 uses a varying number of sectors per track; the short tracks in
the middle of the disk store less. So, it's perfectly normal that those
sectors are missing. This will affect the 'good sectors' score, so it's
normal not to have 100% on this type of disk.)

Clock errors
------------

When FluxEngine sees a track, it attempts to automatically guess the clock
rate of the data in the track. It does this by looking for the magic bit
pattern at the beginning of each sector record and measuring how long it
takes. This is shown in the tracing FluxEngine produces as it runs. For
example:

```
 70.0: 868 ms in 427936 bytes
       138 records, 69 sectors; 2.13us clock; 
       logical track 70.0; 6656 bytes decoded.
 71.0: 890 ms in 387904 bytes
       130 records, 65 sectors; 2.32us clock; 
       logical track 71.0; 6144 bytes decoded.
```

Bits are then found by measuring the interval between pulses on the disk and
comparing to this clock rate.

However, floppy disk drives are extremely analogue devices and not necessarily calibrated very well, and the disk may be warped, or the rubber band which makes the drive work may have lost its bandiness, and so the bits are not necessarily precisely aligned. Because of this, FluxEngine can tolerate a certain amount of error. This is controlled by the `--bit-error-threshold` parameter. Varying this can have magical effects. For example, adding `--bit-error-threshold=0.4` turns the decode into this:

```
H.SS Tracks --->
0. 0 ...B............................................................................
0. 1 ...........B...B................................................................
0. 2 ..............B.................................................................
0. 3 ................................................................................
0. 4 ................................................................................
0. 5 ...B............................................................................
0. 6 ...B.....B......................................................................
0. 7 ...B...........B....B...........................................................
0. 8 ................................................................................
0. 9 .B............B.................................................................
0.10 .B............BB................................................................
0.11 B............B..................................................................
0.12 ...B......B............................................................XXXXXXXXX
0.13 ............B...............................................XXXXXXXXXXXXXXXXXXXX
0.14 ...B......B.....................................XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.15 ..B.....BB..B.........................XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.16 ..B.....B.B.............B..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.17 ..B....B........XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.18 .B..XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Good sectors: 1191/1520 (78%)
Missing sectors: 296/1520 (19%)
Bad sectors: 33/1520 (2%)
80 tracks, 1 heads, 19 sectors, 512 bytes per sector, 760 kB total
```

A drastic difference!

The value of the parameter is the fraction of a clock of error to accept. The
value typically varies from 0.0 to 0.5; the default is 0.2. Larger values
make FluxEngine more tolerant, so trying 0.4 is the first thing to do when
faced with a dubious disk. However, in some situations, increasing the value
can actually _increase_ the error rate --- which is why 0.4 isn't the default
--- so you'll need to experiment.

That's the most common tuning parameter, but others are available:

`--pulse-debounce-threshold` controls whether FluxEngine ignores pairs of pulses in rapid succession. This is common on some disks (I've observed them on Brother word processor disks).

`--clock-interval-bias` adds a constant bias to the intervals between pulses
before doing decodes. This is very occasionally necessary to get clean reads
--- for example, if the machine which wrote the disk always writes pulses
late. If you try this, use very small numbers (e.g. 0.02). Negative values
are allowed.

Both these parameters take a fraction of a clock as a parameter, and you'll
probably never need to touch them.

Clock detection
---------------

A very useful tool for examining problematic disks is `fluxengine inspect`.
This will let you examine the raw flux on a disk (or flux file). It'll also
guess the clock rate on the disk for you, using a simple statistical analysis
of the pulse intervals on the disk. (Note that the tool only works on one
track at a time.)

```
$ fluxengine inspect -s good.flux:t=0:s=0
Clock detection histogram:
3.58    737 ▉
3.67   3838 ████▊
3.75  13078 ████████████████▌
3.83  31702 ████████████████████████████████████████ 
3.92  26682 █████████████████████████████████▋
4.00  22282 ████████████████████████████ 
4.08  12222 ███████████████▍
4.17   4731 █████▉
4.25   1001 █▎
...
7.33    236 ▎
7.42   1800 ██▎
7.50   5878 ███████▍
7.58  10745 █████████████▌
7.67  12442 ███████████████▋
7.75  10144 ████████████▊
7.83   6698 ████████▍
7.92   2779 ███▌
8.00    740 ▉
...
11.17    159 ▏
11.25    624 ▊
11.33   1723 ██▏
11.42   3268 ████ 
11.50   4608 █████▊
11.58   4643 █████▊
11.67   3507 ████▍
11.75   2178 ██▋
11.83    868 █ 
11.92    258 ▎
...
Noise floor:  317
Signal level: 3170
Peak start:   42 (3.50 us)
Peak end:     52 (4.33 us)
Median:       47 (3.92 us)
3.92us clock detected.
```

That's _not_ the histogram from the Victor disk; that's an Apple II disk, and
shows three nice clear spikes (which is very characteristic of the GCR
encoding which the Apple II used). The numbers at the bottom show that a peak
has been detected between 3.50us and 4.33us, and the median is 3.92us, which
corresponds more-or-less with the top of the peak; that's the clock rate
FluxEngine can use.

So, what does my Victor 9000 histogram look like? Let's look at the
histogram for a single track:

```
$ fluxengine inspect -s dubious.flux:t=0:s=0
Clock detection histogram:
1.33   1904 █▉
1.42  21669 ██████████████████████▌
1.50   2440 ██▌
1.58    469 ▍
1.67   7261 ███████▌
1.75   6808 ███████ 
1.83   3088 ███▏
1.92   2836 ██▉
2.00   8897 █████████▎
2.08   6200 ██████▍
...
2.25    531 ▌
2.33   2802 ██▉
2.42   2136 ██▏
2.50   1886 █▉
2.58  10110 ██████████▌
2.67   8283 ████████▌
2.75   7779 ████████ 
2.83   2680 ██▊
2.92  13908 ██████████████▍
3.00  38431 ████████████████████████████████████████ 
3.08  35708 █████████████████████████████████████▏
3.17   5361 █████▌
...
3.75    294 ▎
3.83    389 ▍
3.92   1224 █▎
4.00   3067 ███▏
4.08   4092 ████▎
4.17   6916 ███████▏
4.25  25639 ██████████████████████████▋
4.33  31407 ████████████████████████████████▋
4.42  10209 ██████████▋
4.50   1159 █▏
...
Noise floor:  384
Signal level: 1921
Peak start:   15 (1.25 us)
Peak end:     26 (2.17 us)
Median:       20 (1.67 us)
1.67 us clock detected.
```

That's... not good. The disk is very noisy, and the intervals between pulses
are horribly distributed. The detected clock from the decode is 1.45us, which
does correspond more-or-less to a peak. You'll also notice that the
double-clock interval is at 3.00us, which is _not_ twice 1.45us. The guessed
clock by `fluxengine inspect` is 1.67us, which is clearly wrong.

This demonstrates that the statistical clock guessing isn't brilliant, which
is why I've just rewritten the decoder not to use it; nevertheless, it's a
useful tool for examining disks.

`fluxengine inspect` will also dump the raw flux data in various formats, but
that's mostly only useful to me. Try `--dump-bits` to see the raw bit pattern
on the disk (using the guessed clock, or `--manual-clock-rate-us` to set it
yourself); `--dump-flux` will show you discrete pulses and the intervals
between them.

The tool's subject to change without notice as I tinker with it.
