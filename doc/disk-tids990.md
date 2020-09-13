Disk: TI DS990 FD1000
=====================

The Texas Instruments DS990 was a multiuser modular computing system from 1998,
based around the TMS-9900 processor (as used by the TI-99). It had an 8" floppy
drive module, the FD1000, which was a 77-track, 288-byte sector FM/MFM system
with 26 sectors per track. The encoding scheme was very similar to a simplified
version of the IBM scheme, but of course not compatible. A double-sided disk
would store a very satisfactory 1126kB of data; here's one at <a
href="https://www.old-computers.com/museum/computer.asp?st=1&c=1025">old-computers.com</a>:

<div style="text-align: center">
<a href="https://www.old-computers.com/museum/computer.asp?st=1&c=1025">
<img src="tids990.jpg" style="max-width: 60%" alt="A DS990 at old-computers.com"></a>
</div>

FluxEngine will read and write these (but only the DSDD MFM variant).

Reading discs
-------------

Just do:

```
fluxengine read tids990
```

You should end up with an `tids990.img` which is 1153152 bytes long.

Writing discs
-------------

Just do:

```
fluxengine write tids990 -i tids990.img
```


Useful references
-----------------

  - [The FD1000 Depot Maintenance
	Manual](http://www.bitsavers.org/pdf/ti/990/disk/2261885-9701_FD1000depotVo1_Jan81.pdf)


