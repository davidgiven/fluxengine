Using the FluxEngine client software with Applesauce hardware
===============================================================

The FluxEngine isn't the only project which does this; another one is the
[Applesauce](https://applesaucefdc.com/), a proprietary but feature-rich
off-the-shelf imaging device. Its native client (which is a lot better than
FluxEngine) only works on OSX, so if you want to use it anywhere else,
the FluxEngine client works.

The Applesauce works rather differently to the FluxEngine hardware or the
[Greaseweazle](greaseweazle.md), so there are some caveats.

 - Rather than streaming the flux data from the device to the PC, the Applesauce
   has a fixed buffer in RAM used to capture a complete image of a track. This is
   then downloaded later. The advantage is that USB bandwidth isn't an issue; the
   downside is that the buffer can only hold so much data. In fact, the Applesauce
   can only capture 1.25 revolutions or 2.25 revolutions, nothing else. When used
   with the FluxEngine the capture time will be ignored apart from used to
   determine whether you want a 'long' or 'short' capture.

 - The current (v2) firmware only supports reading, not writing (via clients
   other than the official one, of course). The new (v3) firmware will support
   writing, but it's not out yet, so for the time being the FluxEngine client is
   read only.

 - You can only do synchronous reads, i.e., reads starting from the index mark.

Other than this, the FluxEngine software supports the Applesauce almost
out-of-the-box --- just plug it in and nearly everything should work. The
FluxEngine software will autodetect it. If you have more than one device plugged
in, use `--usb.serial=` to specify which one you want to use.

I am aware that having _software_ called FluxEngine and _hardware_ called
FluxEngine makes things complicated when you're not using the FluxEngine client
software with a FluxEngine board, but I'm afraid it's too late to change that
now. Sorry.

What works
----------

Supported features with the Greaseweazle include:

  - simple reading of disks, seeking etc
  - erasing disks
  - hard sectored disks
  - determining disk rotation speed
  - normal IBM buses

I don't know what happens if you try to use an Apple Superdrive or a Apple II
disk with FluxEngine. If you've got one, [please get in
touch](https://github.com/davidgiven/fluxengine/issues/new)!

What doesn't work
-----------------

  - voltage measurement
  - writing

Who to contact
--------------

I want to make it clear that the FluxEngine code is _not_ supported by the
Applesauce team. If you have any problems, please [contact
me](https://github.com/davidgiven/fluxengine/issues/new) and not them.

In addition, the Applesauce release cycle is not synchronised to the
FluxEngine release cycle, so it's possible you'll have a version of the
Applesauce firmware which is not supported by FluxEngine. Hopefully, it'll
detect this and complain. Again, [file an
issue](https://github.com/davidgiven/fluxengine/issues/new) and I'll look into
it.

