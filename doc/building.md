Building the hardware
=====================

This page documents what you need to do to actually build one. Do not fear, I
know several people with mediocre soldering skills who've done it (I count
myself as one of those).

## Bill of materials

This is the physical stuff you'll need.

  - One or more floppy disk drives. Both 3.5" and 5.25" work. One FluxEngine
    will even run both drives (but not at the same time, obviously). You'll
    also need the appropriate cabling to plug the drives into a PC.

  - A [Cypress PSoC5LP CY8CKIT-059 development
    board](http://www.cypress.com/documentation/development-kitsboards/cy8ckit-059-psoc-5lp-prototyping-kit-onboard-programmer-and),
    which is a decently fast ARM core wrapped around a CLDC/FPGA soft logic
    device. You can get one directly from Cypress via the link above for $10,
    but shipping can be extortionate depending where you are. You can also
    find them on eBay or Amazon for about $20.

  - **Either** a 17-way header pin strip **or** [a
    34-way IDC motherboard
    connector](https://eu.mouser.com/ProductDetail/Amphenol-FCI/86130342114345E1LF?qs=%2Fha2pyFadug%252BpMTyxmFhglPPVKuWXYuFpPNgq%252BsrzhDnXxo8B28k7UCGc7F%2FXjsi)
    (or one of the other myriad compatible connectors; there's a billion).

  - A suitable power supply. 3.5" floppy drives use 5V at about an amp
    (usually less) --- sadly, too much to power from USB. 5.25" floppy drives
    also require 12V. An old but decent quality PC power supply is ideal, as
    it'll frequently come with the right connectors.

  - a Windows machine to run the Cypress SDK on. (The FluxEngine client
    software itself will run on Linux, Windows, and OSX, but you
    have to build the firmware on Windows.)

  - Basic soldering ability.

  - (Optional) Some kind of box to put it in. I found an old twin 5.25"
	Hewlett Packard drive enclosure and ripped all the SCSI guts out; this not
	only provides a good, solid box to house both my 3.5" and 5.25" drives in,
	but also contains an ideal power supply too. Bonus!


## Assembly instructions

All you need to do is attach your chosen connector to the board. You'll need
to make sure that pin 2 on the cable is connected to pin 2.7 on the board,
and pin 34 to pin 1.7 on the board (and of course all the ones in between).
Apart from grounding the board (see below), this is literally all there is to
it. The actual pinout is described in detail below.

The pads are small, but soldering them isn't too bad with a needle-nosed
soldering iron tip.

### If you're using a connector

Line it up like this.

<div style="text-align: center">
<a href="closeup1.jpg"><img src="closeup1.jpg" style="width:80%" alt="closeup of the board with connector attached"></a>
</div>

Note the following:

  - You're looking at the back of the board (the side without the big square
    chips). The connector sticks out of the front.

  - The notch on the connector goes at the top.

  - The top row of pins on the connector overhang the edge of the board: they remain unconnected.

Also, be aware that some floppy disk cables don't have a projection on the
motherboard end to fit into that notch, and so will plug in either way round.
That's fine, but some of these get round this by missing a hole for pin 5.
That way they'll only plug into the connector one way round, because the
connector is missing a pin. If you have one of these cables (I do), you'll
need to use a pair of needle-nosed pliers to pull pin 5 out of the connector.

### If you're using header pins

Line it up like this.

<div style="text-align: center">
<a href="closeup2.jpg"><img src="closeup2.jpg" style="width:80%" alt="closeup of the board with connector attached"></a>
</div>

You're now looking at the _top_ of the board.

(It's also possible to put the pins on the bottom of the board, or to use a
row of header sockets allowing you to plug the board directly onto the floppy
disk drive; for simplicity I'm leaving that as an exercise for the reader.)

### Grounding

You _also_ need to solder a wire between a handy GND pin on the board and
connect it to ground on the drive. Because the board is powered by USB and
the drive by your external power supply, they can be at different potentials,
and they need to be tied together.

If you're using a connector, the simplest thing to do is to bend up one of
the unconnected pins and solder a short piece of wire to a GND pin on the
board. Alternatively you'll need to splice it into your drive's power supply
cable somehow. (The black one.)

## Programming the board

You've got two options here. You can either use the precompiled firmware
supplied with the source, or else install the Cypress SDK and build it
yourself. If you want to hack the firmware source you need the latter, but
if you trust me to do it for you use the precompiled firmware. In either
case you'll need Windows and have to install some Cypress stuff.

**Before you read this:** If you're on Windows, good news! You can download a
precompiled version of the FluxEngine client and precompiled firmware [from
the GitHub releases
page](https://github.com/davidgiven/fluxengine/releases/latest). Simply unzip
it somewhere and run the `.exe` files from a `cmd` window (or other shell).
Follow the instructions below to program the board with the firmware.

### Using the precompiled firmware

On your Windows machine, [install the PSoC
Programmer](https://www.cypress.com/products/psoc-programming-solutions).
**Note:** _not_ the Cypress Programmer, which is for a different board!
Cypress will make you register.

Once done, run it. Plug the blunt end of the FluxEngine board into a USB
port (the end which is a USB connector). The programmer should detect it
and report it as a KitProg. You may be prompted to upgrade the programmer
hardware; if so, follow the instructions and do it.

Now go to File -> File Load and open
`FluxEngine.cydsn/CortexM3/ARM_GCC_541/Release/FluxEngine.hex` in the
project. If you're on Windows, the precompiled zipfile also contains a copy
of this file. Press the Program button (the one in the toolbar marked with a
down arrow). Stuff will happen and you should be left with three green boxes
in the status bar and 'Programming Succeeded' at the top of the log window.

You're done. You can unplug the board and close the programmer.

### Building the firmware yourself

On your Windows machine, [install the Cypress SDK and CY8CKIT-059
BSP](http://www.cypress.com/documentation/development-kitsboards/cy8ckit-059-psoc-5lp-prototyping-kit-onboard-programmer-and).
This is a frustratingly long process and there are a lot of moving parts; you
need to register. You want the file from the above site marked 'Download
CY8CKIT-059 Kit Setup (Kit Design Files, Creator, Programmer, Documentation,
Examples)'. I'm not linking to it in case the URL changes when they update
it.

Once this is done, I'd strongly recommend working through the initial
tutorial and making the LED on your board flash. It'll tell you where all the
controls are and how to program the board. Remember that the big end of the
board plugs into your computer for programming.

When you're ready, open the `FluxEngine.cydsn/FluxEngine.cyprj` project,
pick 'Program' from the menu, and the firmware should compile and be
programmed onto your board.

**Note:** If programming doesn't work and you get a strange dialogue
box asking about port acquisition, then this is because the device isn't
responding to the programmer. This is normal but annoying. You should see the
device in the dialogue. Select it and press the 'Port Acquire' button. The
device should reset and an extra item will appear in the dialogue; select
this and press OK.

If acquiring the port doesn't work, resulting in the IDE hanging for 45
seconds and then producing a meaningless error message, you need to reset the
programmer (the little board hanging off the side of the bigger board).
You'll see that the light on the programmer is pulsing slowly in a breathing
pattern. Press and hold the little button near the light for five seconds
until the light stays solidly on. Now you should be able to acquire
the port and proceed normally.

## Technical details

The board pinout and the way it's connected to the floppy bus is described
below.

```ditaa
:-E -s 0.75
                 +-----+
                 |||||||
            +----+-----+----+
            +cAAA           +
            +  Debug board  +
            +----+-----+----+
            + GND|cDDD | VDD+  
            +----+     +----+
INDEX300 ---+ 3.0|     | GND+--------------------------+
            +----+     +----+                 +--+--+  |
INDEX360 ---+ 3.1|     | 1.7+------ DISKCHG --+34+33+--+
            +----+     +----+                 +--+--+
            + 3.2|     | 1.6+------- SIDE1 ---+32+31+
            +----+     +----+                 +--+--+
            + 3.3|     | 1.5+------- RDATA ---+30+29+
            +----+     +----+                 +--+--+
            + 3.4|     | 1.4+-------- WPT ----+28+27+
            +----+     +----+                 +--+--+
            + 3.5|     | 1.3+------- TRK00 ---+26+25+
            +----+     +----+                 +--+--+
            + 3.6|     | 1.2+------- WGATE ---+24+23+
            +----+     +----+                 +--+--+
            + 3.7|     | 1.1+------- WDATA ---+22+21+
            +----+     +----+                 +--+--+
            +15.0|     | 1.0+------- STEP ----+20+19+
            +----+     +----+                 +--+--+
            +15.1|     |12.0+-------- DIR ----+18+17+
            +----+     +----+                 +--+--+
            +15.2|     |12.1+------- MOTEB ---+16+15+
            +----+     +----+                 +--+--+
            +15.3|     |12.2+------- DRVSA ---+14+13+
            +----+     +----+                 +--+--+
            +15.4|     |12.3+------- DRVSB ---+12+11+
            +----+     +----+                 +--+--+
            +15.5|     |12.4+------- MOTEA ---+10+9 +
            +----+     +----+                 +--+--+
            + 0.0|     |12.5+------- INDEX ---+8 +7 +
            +----+     +----+                 +--+--+
            + 0.1|     |12.6+-------- n/c ----+6 +5 +
            +----+     +----+                 +--+--+
            + 0.2|     |12.7+- TX --- n/c ----+4 +3 +
            +----+     +----+                 +--+--+
            + 0.3|     | 2.7+------- REDWC ---+2 +1 +
            +----+     +----+                 +--+--+
            + 0.4|     | 2.6+  
            +----+     +----+                FDD socket
            + 0.5|     | 2.5+  
            +----+     +----+
            + 0.6|     | 2.4+    TX: debug UART from board
            +----+     +----+
            + 0.7|     | 2.3+
            +----+     +----+
            + RST|     | 2.2+  
            +----+     +----+
            + GND|     | 2.1+  
            +----+ USB +----+
            + VDD+-----+ 2.0+  
            +----+-----+----+
               PSoC5 board
```

Notes:

  - `TX` is the debug UART port. It's on pin 12.7 because the board routes it
  to the USB serial port on the programmer, so you can get debug information
  from the FluxEngine by just plugging the programming end into a USB port
  and using a serial terminal at 115200 baud. If you solder a floppy drive
  connector on, then it'll end up connected to pin 4 of the floppy drive bus,
  which is usually not connected. It's possible that some floppy drives do,
  in fact, use this pin. You may wish to remove pin 4 from the floppy drive
  socket before attaching it to the FluxEngine to make sure that this pin is
  not connected; however, so far I have not found any drives for which this
  is necessary. If you do find one, _please_ [get in
  touch](https://github.com/davidgiven/fluxengine/issues/new) so I can
  document it.

  - The `GND` pin only really needs to be connected to one of the floppy bus
  ground pins; pin 33 is the closest. For extra safety, you can bridge all
  the odd numbered pins together and ground them all if you like.

  - `INDEX300` and `INDEX360` are optional output pins which generate fake
  timing pulses for 300 and 360 RPM drives. These are useful for certain
  rather exotic things. See the section on flippy disks [in the FAQ](faq.md)
  for more details; you can normally ignore these.

## Building the client

The client software is where the intelligence, such as it is, is. It's pretty
generic libusb stuff and should build and run on Windows, Linux and OSX as
well, although on Windows it'll need MSYS2 and mingw32. You'll need to
install some support packages.

  - For Linux (this is Ubuntu, but this should apply to Debian too):
  `ninja-build`, `libusb-1.0-0-dev`, `libsqlite3-dev`.
  - For OSX with Homebrew: `ninja`, `libusb`, `pkg-config`, `sqlite`.
  - For Windows with MSYS2: `make`, `ninja`, `mingw-w64-i686-libusb`,
  `mingw-w64-i686-sqlite3`, `mingw-w64-i686-zlib`, `mingw-w64-i686-gcc`.

These lists are not necessarily exhaustive --- plaese [get in
touch](https://github.com/davidgiven/fluxengine/issues/new) if I've missed
anything.

All systems build by just doing `make`. You should end up with a single
executable in the current directory, called `fluxengine`. It has minimal
dependencies and you should be able to put it anywhere.

If it doesn't build, please [get in
touch](https://github.com/davidgiven/fluxengine/issues/new).

## Next steps

The board's now assembled and programmed. Plug it into your drive, strip the
plastic off the little USB connector and plug that into your computer, and
you're ready to start using it.

I _do_ make updates to the firmware whenever necessary, so you may need to
reprogram it at intervals; you may want to take this into account if you
build a case for it. I have a USB extension cable plugged onto the programmer
port, which trails out the side of my drive enclosure. This works fine.
