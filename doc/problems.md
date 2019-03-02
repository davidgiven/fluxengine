My disk won't read properly!
============================

So you're trying to read a disk and it's not working. Well, you're not the only one. Floppy disks are unreliable and awkward things. FluxEngine can help, but the tools aren't very user friendly.

The sector map
--------------

Every time I do a read, FluxEngine will give me a dump like this:

```
H.SS Tracks --->
0. 0 XBBXXXXXXXXXBXBBXBBXBBX..XXX.BXBBBXXX....BB...BB...........B.XXXX.X....BBBBBBXBB
0. 1 X.BXXXXBBXXX.XBBXBXXBBB.XXXX.BBXBXXBX....BB...BX........BB.B.XXXX.X....BBBBBBXBX
0. 2 X..XXXXXBXXX.XBBXXBXBBB.BXXB.BXBBBXXX...BXX.B.BB.........B.B.XXXX.X....BBBBBBXBX
0. 3 X.BXXXXXXXXX.X.XXBBXBBX.XXXX.XXXXXXXX...BBX.X.XX......B....B.XXXB.X....BBBXBBXBX
0. 4 X.BXXXXXXXXX.XBXXBXXBBB.XXXX.BXXXXXXX...BBB.X.BX......B....B.XXXX.X....BBBBBBXBX
0. 5 X.BXXXXXXXXX.XBXXBBXBB.BXXXX.XXXXXXXX...XBB.X.XX.....B.....B.XXXX.B....BBBXBBXBX
0. 6 X..XXXXBXXXX.X.XXBBXBBB.XXXB.XXBBBXXX..B.BB.B.XB.............XXXX.X....BBBXBBXBX
0. 7 X..XXXXBXXXXBX.XXBBXBBB.BXXX.XXXBXXXX..BBXX.X.XX......B....B.XXXX.X....BBBXBBXBX
0. 8 X.BXXXXBXXXXBX.XXBBXBBX.XXXX.XXXXXXXX..BXXX.X.XX......BB.B...XXXX.X....BBBBXBXBX
0. 9 X.BXXXXXXXXXBX.XXBBXBBB.XXXX.XXXXXXXX..XXXX.X.XX......X......XXXX.X....BBBBBBXBX
0.10 X..XXXXBXXXXBX.XXBBXXB..BXXXBBXXBXXXX..BBXB.B.BB.............XXXX.X....BBBBBXXBX
0.11 X..XXXXXXXXXBX.XXBBXBBB.XXXX.BXXXXXXX..XXXX.X.BB....B........XXXX.X....BBBBBBBBB
0.12 X.BXXXXXXXXXBX.XXBXXXXB.XXXX.XXXXXXXX..XXXX.X.XX.........B.B.BXXX.B....XXXXXXXXX
0.13 X..XBXXXXXXXBX.XXBBXBBB.XXXX.XXXXXXXX..XXXX.X.XX....B.B.BB.BXXXXXXXXXXXXXXXXXXXX
0.14 XB.XXXXXXXXXBX.XXBBXBBB.XXXB.XXBXXXXB..BBB..B.BBXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.15 X..XXXXXXXXXBX.XXBBXBBB.XXXX.XXXXXXXX.XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.16 X..XXBXXBXBXBBBXXBBXBBB.XXBXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.17 XBBXXXXB.XXXBXBXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.18 XBBXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Good sectors: 369/1520 (24%)
Missing sectors: 847/1520 (55%)
Bad sectors: 304/1520 (20%)
80 tracks, 1 heads, 19 sectors, 512 bytes per sector, 760 kB total
```

This is the **sector map**, and is showing me the status of every sector it
found on the disk. (Tracks on the X-axis, sectors on the Y-axis.) This is a
very bad read from a [Victor 9000](victor9k.html) disk; good reads shouldn't
look like this. A dot represents a good sector. A B is one where the CRC
check failed; an X is one which couldn't be found at all.

At the very bottom there's a summary: 24% good sectors. Let me try and improve
that.

(You may notice the wedge of Xs in the bottom right. This is because the
Victor 9000 uses a varying number of sectors per track; the short tracks in
the middle of the disk store less. So, it's perfectly normal that those
sectors are missing. This will affect the 'good sectors' score, so it's
normal not to have 100% on this disk.)

Clock detection
---------------

When FluxEngine sees a track, it attempts to automatically guess the clock
rate of the data in the track. It does this by computing a histogram of the
spacing between pulses and attempting to detect the shortest peak. The
histogram should look something like this.

```
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
$ fe-readvictor -s diskimage/:s=0:t=0 --show-clock-histogram
Reading from: diskimage/:d=0:s=0:t=0
  0.0: 829 ms in 316193 bytes
Clock detection histogram:
1.25    447 ▌
1.33  16283 ████████████████████▎
1.42  22879 ████████████████████████████▌
1.50   4564 █████▋
1.58   1272 █▌
1.67  19594 ████████████████████████▍
1.75  32059 ████████████████████████████████████████ 
1.83  18042 ██████████████████████▌
1.92   2249 ██▊
2.00   8825 ███████████ 
2.08  16031 ████████████████████ 
2.17   5080 ██████▎
2.25    409 ▌
2.33   7216 █████████ 
2.42   6269 ███████▊
2.50    514 ▋
2.58   5176 ██████▍
2.67  13080 ████████████████▎
2.75   6774 ████████▍
2.83   1916 ██▍
2.92  10880 █████████████▌
3.00  21277 ██████████████████████████▌
3.08  11625 ██████████████▌
3.17   1028 █▎
...
3.67   1910 ██▍
3.75  19720 ████████████████████████▌
3.83  12365 ███████████████▍
3.92    814 █ 
4.00   3144 ███▉
4.08   5776 ███████▏
4.17   3278 ████ 
4.25   8487 ██████████▌
4.33  15922 ███████████████████▊
4.42   9656 ████████████ 
4.50   1540 █▉
...
Noise floor:  320
Signal level: 3205
Peak start:   14 (1.17 us)
Peak end:     39 (3.25 us)
Median:       23 (1.92 us)
```

That's... not good. The disk is very noisy, and the intervals between pulses
are horribly distributed. The detected clock is 1.92us, which is clearly
wrong.

I can override the clock detection and specify the clock manually. 1.75us
looks like a good candidate. Let's try that on track 0.

```
$ fe-readvictor9k -s diskimage/:s=0:t=0 --manual-clock-rate-us=1.75
...skipped...
No sectors in output; skipping analysis
0 tracks, 0 heads, 0 sectors, 0 bytes per sector, 0 kB total
```

Nope, nothing --- FluxEngine was unable to find any valid data. How about
1.42us, this time for the whole disk?

```
$ fe-readvictor9k -s diskimage/:s=0:t=0 --manual-clock-rate-us=1.42
...skipped...
H.SS Tracks --->
0. 0 .BBBBBBBBXBBBBBBXXXXXXXXXBB
0. 1 B.BBBBBBBXXXXBBBXXXXXXXXXXX
0. 2 B..BBBBBBBBBXBBBXXXXXXXXXXX
0. 3 B.BBBBBBBXBB.BBBXXXXXXXXXXX
0. 4 B.BBBBBBBXBB.BBBXBXXXXXXXBB
0. 5 B.BBBBBBBBBB.BXBXXXXXXXXXBB
0. 6 B..BBBBBBXBBXBXBXXXXXXXXXXX
0. 7 B..BBBBBBBBBXB.XXXBBXXXBXBX
0. 8 B.BBBBBBBBBBXB.BXBBXXXX.XXX
0. 9 B.BBBBBBBXXXXX.BXXXXXXXXXXX
0.10 B..BBBBBBBBBXB.BXXXXXXXXXXX
0.11 B..BBBBBXXBXBB.BXXXXXXXXXXX
0.12 B.BBBBBB.BXBBB.BXXXXXXXXXXX
0.13 B..BBBBB.BBBBB.BXXBXXXXXXXX
0.14 BB.BBBBBXBBBBB.BXBXXXXXXXXX
0.15 B..BBBBB.XBBBB.BXXXXXXXXXXX
0.16 B..BBBBBBXBBXBBBXBBXXXXXXXX
0.17 BBBBBBBB.XBBXBBBXXXXXXXXXXX
0.18 BBBBXXXXXXXXXXXXXXXXXXXXXXX
Good sectors: 42/513 (8%)
Missing sectors: 234/513 (45%)
Bad sectors: 237/513 (46%)
27 tracks, 1 heads, 19 sectors, 512 bytes per sector, 256 kB total
```

It found something. The sectors in track 0 are now B rather than X, which
means that FluxEngine at least found them, and look, one sector even passed
its CRC!

But it turns out that the Victor 9000 actually uses a varying clock rate from
track to track, so as to fit more data on the longer tracks on the outside of
the disk. So, manually setting the clock rate to 1.42us has actually made
things _worse_ at the other end of the disk. Our overall bad sector rate has
gone up from 20% to 46%.

So, I look back at the histogram. I want to keep using the clock
autodetection, but persuade it to detect the right clock. There _is_ a peak
at 1.42us, but there's enough noise around it to confuse the peak detection
algorithm. You can see from the summary at the end that it thinks the peak
extends from 1.17us to 3.25us.

The way to correct this is to change the noise floor. This makes it ignore
frequencies below a certain level. Raising this will make it much more
conservative about what it considers a frequency peak. With good data, this
actually makes frequency detection _less_ accurate, but with bad data it can
help.

```
$ fe-readvictor9k -s diskimage/:s=0:t=0  --show-clock-histogram --noise-floor-factor=0.25
Reading from: diskimage/:d=0:s=0:t=0
  0.0: 829 ms in 316193 bytes
Clock detection histogram:
1.33  16283 ████████████████████▎
1.42  22879 ████████████████████████████▌
1.50   4564 █████▋
...
1.67  19594 ████████████████████████▍
1.75  32059 ████████████████████████████████████████ 
1.83  18042 ██████████████████████▌
...
2.00   8825 ███████████ 
2.08  16031 ████████████████████ 
2.17   5080 ██████▎
...
...skipped...
Noise floor:  8014
Signal level: 3205
Peak start:   15 (1.25 us)
Peak end:     18 (1.50 us)
Median:       17 (1.42 us)
...skipped...
Good sectors: 1/19 (5%)
Missing sectors: 0/19 (0%)
Bad sectors: 18/19 (94%)
1 tracks, 1 heads, 19 sectors, 512 bytes per sector, 9 kB total
```

So, it's now found a peak from 1.25us to 1.50us, with a median of 1.42us ---
exactly what I wanted. Let's try it on the whole disk:

```
H.SS Tracks --->
0. 0 .BBBBBBBBBBBBBBBB.BBB.B..BB.....................................................
0. 1 B.BBBBBBBBXXX.BBB.BBB.X..BB..........B..........................................
0. 2 B..BBBBBB.BBXBBBB.BBB.X..BB.......B.............................................
0. 3 B.BBBBBBBBBB.B.BX.BBB.B..BB......B..............................................
0. 4 B.BBBBBB.BBB.BBBB.BBB.B..BB.....B...............................................
0. 5 B.BBBBBBBBBB.BBBB.BBB.XB.BB.....B.B.............................................
0. 6 B..BBBBBBBBBXB.BX.BBB.X.XXB.....................................................
0. 7 B..BBBBBBBBBXB.XB.BBB.X.BBB.....................................................
0. 8 B.BBBBBBBBBBXB.BB.BBB.X.XXX.B...................................................
0. 9 B.BBBBBBBBXXX..BX.BXB.X.XXXBB...................................................
0.10 B..BBBBB.BBBXB.BB.BBB...BBB.B...................................................
0.11 B..BBBBB.BBXBB.BB.BXB.B.BBB.B......B............................................
0.12 B.BBBBBB.BXBBB.BB.BXX.X.XXX........B...................................XXXXXXXXX
0.13 B..BBBBB.BBBBB.BX.BBB.X.BXB......BB.........................XXXXXXXXXXXXXXXXXXXX
0.14 BB.BBBBB..BBBB.BB.BBB.B.XBB.B....BB.B...........XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.15 B..BBBBB.BBBBB.BB.BBB.X.XBB.B....B....XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.16 B..BBBBB.BBBXBBBB.BBB.X.BXBXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.17 BBBBBBBB.BBBX.BBXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
0.18 BBBBXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
Good sectors: 834/1520 (54%)
Missing sectors: 346/1520 (22%)
Bad sectors: 340/1520 (22%)
80 tracks, 1 heads, 19 sectors, 512 bytes per sector, 760 kB total
```

54% good sectors --- much better! Most of the top half of the disk is reading
flawlessly. The bottom half is still dreadful, but much better.

I picked this disk as a sample because it's essentially wrecked. It's the
worst disk image I've ever seen. Luckily I didn't scan this myself, because
chances are it's all mouldy and would wreck my disk heads. But its very
badness makes it a good example.

(I am continually improving the clock detection and data extraction
algorithms. I have actually seen someone get more data than I off this image,
with a mostly-good read of track 0. I must find out their secrets...)
