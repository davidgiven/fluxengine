psos
====
## 800kB DSDD with PHILE
<!-- This file is automatically generated. Do not edit. -->

pSOS was an influential real-time operating system from the 1980s, used mainly
on 68000-based machines, lasting up until about 2000 when it was bought (and
cancelled) by Wind River. It had its own floppy disk format and file system,
both of which are partially supported here.

The PHILE file system is almost completely undocumented and so many of the data
structures have had to be reverse engineered and are not well known. Please
[get in touch](https://github.com/davidgiven/fluxengine/issues/new) if you know
anything about it.

The floppy disk format itself is an IBM scheme variation with 1024-byte sectors
and, oddly, swapped sides.

## Options

(no options)

## Examples

To read:

  - `fluxengine read psos -s drive:0 -o pme.img`

To write:

  - `fluxengine write psos -d drive:0 -i pme.img`

