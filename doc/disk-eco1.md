Disk: VDS Eco1
==============

The Eco1 is a Italian CP/M machine produced in 1982. It used twin 8" drives,
each storing 1.2MB, which was quite impressive for a CP/M machine in those
days. Visually it is best described as 'very brown'.

<div style="text-align: center">
<a href="vds-eco1.jpg"> <img src="vds-eco1.jpg" alt="A contemporary advert for the Eco1"/></a>
</div>

Its format is standard IBM scheme, but with an interesting wrinkle: there are
_three_ different formatting zones on the disk:

  - Track 0 side 0: 26 sectors, 128 bytes per sector (3296 bytes)
  - Track 0 side 1: 26 sectors, 256 bytes per sector (6656 bytes)
  - All others: 16 sectors, 512 bytes per sector (8192 bytes)

The standard `read ibm` command will autodetect and read these disks, but due
to the format confusing the size autodetection the images need postprocessing
to be useful, so there's a custom profile for the Eco1 which produces sensible
images.

Reading discs
-------------

Just do:

```
fluxengine read eco1 -o eco1.img
```

You should end up with an `eco1.img` of 1255168 bytes, containing all the
sectors concatenated one after the other in CHS order, with no padding.

References
----------

  - [Apulio Retrocomputing's page on the Eco1](https://www.apuliaretrocomputing.it/wordpress/?p=8976)

