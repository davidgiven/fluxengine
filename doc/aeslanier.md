AES Lanier word processor disks
===============================

Back in 1980 Lanier released a series of very early integrated word processor
appliances, the No Problem. These were actually [rebranded AES Data Superplus
machines](http://vintagecomputers.site90.net/aes/). They wrer gigantic,
weighed 40kg, and one example I've found cost £13,000 in 1981 (the equivalent
of nearly £50,000 in 2018!).

8080 machines with 32kB of RAM, they ran their own proprietary word
processing software off twin 5.25" drive units, but apparently other software
was available.

The disk format is exceptionally weird. They used hard sectored disks, where
there were multiple index holes, one for each sector. The encoding scheme
itself is [MMFM (aka
M2FM)](http://www.retrotechnology.com/herbs_stuff/m2fm.html), an early
attempt at double-density disk encoding which rapidly got obsoleted by the
simpler MFM. Even aside from the encoding, the format on disk was strange;
unified sector header/data records, and 253 (or maybe 252) byte sectors.

FluxEngine can read these, but I only have a single, fairly poor example of a
disk image, and I've had to make a lot of guesses as to the sector format
based on what looks right. If anyone knows _anything_ about these disks,
[please get in touch](https://github.com/davidgiven/fluxengine/issues/new).

Reading discs
-------------

Just do:

```
.obj/fe-readaeslanier
```

Useful references
-----------------

  * [SA800 Diskette Storage Drive - Theory Of Operations](http://www.hartetechnologies.com/manuals/Shugart/50664-1_SA800_TheorOp_May78.pdf): talks about MMFM a lot, but the Lanier machines didn't use this disk format.