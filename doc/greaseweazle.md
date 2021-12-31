Using the FluxEngine client software with GreaseWeazle hardware
===============================================================

The FluxEngine isn't the only project which does this; another one is the
[GreaseWeazle](https://github.com/keirf/Greaseweazle/wiki), a Blue Pill based
completely open source solution. This requires more work to set up (or you can
buy a prebuilt GreaseWeazle board), but provides completely open source
hardware which doesn't require the use of the Cypress Windows-based tools that
the FluxEngine does. Luckily, the FluxEngine software supports it almost
out-of-the-box --- just plug it in and nearly everything should work. The
FluxEngine software will autodetect it. If you have more than one device
plugged in, use `--usb.serial=` to specify which one you want to use.

I am aware that having _software_ called FluxEngine and _hardware_ called
FluxEngine makes things complicated when you're not using the FluxEngine client
software with a FluxEngine board, but I'm afraid it's too late to change that
now. Sorry.

**If you were using a previous version on Windows** you might have installed
the WinUSB driver. That's no longer needed, and will in fact not work. You'll
need to use Zadig to restore the old driver; to do this, make sure the left
Driver box says `WinUSB` and the right one says `USB Serial (CDC)`. Then press
`Replace Driver`. You won't need Zadig any more.

What works
----------

Supported features with the GreaseWeazle include:

  - simple reading and writing of disks, seeking etc
  - erasing disks
  - determining disk rotation speed

What doesn't work
-----------------

(I'm still working on this. If you have an urgent need for anything, please
[file an issue](https://github.com/davidgiven/fluxengine/issues/new) and I'll
see what I can do.)

  - voltage measurement
  - hard sectored disks (you can still read these, but you can't use
	`--fluxsource.drive.hard_sector_count`).

Who to contact
--------------

I want to make it clear that the FluxEngine code is _not_ supported by the
GreaseWeazle team. If you have any problems, please [contact
me](https://github.com/davidgiven/fluxengine/issues/new) and not them.

In addition, the GreaseWeazle release cycle is not synchronised to the
FluxEngine release cycle, so it's possible you'll have a version of the
GreaseWeazle firmware which is not supported by FluxEngine. Hopefully, it'll
detect this and complain. Again, [file an
issue](https://github.com/davidgiven/fluxengine/issues/new) and I'll look into
it.

