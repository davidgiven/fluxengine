Using the FluxEngine client software with Greaseweazle hardware
===============================================================

The FluxEngine isn't the only project which does this; another one is the
[Greaseweazle](https://github.com/keirf/Greaseweazle/wiki), a Blue Pill based
completely open source solution. This requires more work to set up (or you can
buy a prebuilt Greaseweazle board), but provides completely open source
hardware which doesn't require the use of the Cypress Windows-based tools that
the FluxEngine does. Luckily, the FluxEngine software supports it almost
out-of-the-box --- just plug it in and nearly everything should work. The
FluxEngine software will autodetect it. If you have more than one device
plugged in, use `--usb.serial=` to specify which one you want to use.

I am aware that having _software_ called FluxEngine and _hardware_ called
FluxEngine makes things complicated when you're not using the FluxEngine client
software with a FluxEngine board, but I'm afraid it's too late to change that
now. Sorry.

**If you are using Greaseweazle-compatible hardware** such as the
[adafruit-floppy](https://github.com/adafruit/Adafruit_Floppy) project, then
FluxEngine will still work; however, as the USB VID/PID won't be that of a real
Greaseweazle, the the FluxEngine client can't autodetect it. Instead, you'll
need to specify the serial port manually with something like
`--usb.greaseweazle.port=/dev/ttyACM0` or `--usb.greaseweazle.port=COM5`.

**If you were using a previous version on Windows** you might have installed
the WinUSB driver. That's no longer needed, and will in fact not work. You'll
need to use Zadig to restore the old driver; to do this, make sure the left
Driver box says `WinUSB` and the right one says `USB Serial (CDC)`. Then press
`Replace Driver`. You won't need Zadig any more.

What works
----------

Supported features with the Greaseweazle include:

  - simple reading and writing of disks, seeking etc
  - erasing disks
  - determining disk rotation speed
  - Shugart and normal IBM buses (via
	`--usb.greaseweazle.bus_type=SHUGART` or `IBMPC`; the default is `IBMPC`)
  - Apple 5.25 floppy interfaces (via `--usb.greaseweazle.bus_type=APPLE2`)

Which device types are supported depend on the hardware. Genuine Greaseweazle
hardware supports SHUGART and IBMPC.  APPLE2 is only supported with hand wiring
and the Adafruit\_Floppy greaseweazle-compatible firmware.

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
Greaseweazle team. If you have any problems, please [contact
me](https://github.com/davidgiven/fluxengine/issues/new) and not them.

In addition, the Greaseweazle release cycle is not synchronised to the
FluxEngine release cycle, so it's possible you'll have a version of the
Greaseweazle firmware which is not supported by FluxEngine. Hopefully, it'll
detect this and complain. Again, [file an
issue](https://github.com/davidgiven/fluxengine/issues/new) and I'll look into
it.

