The sampling system is dumb as rocks.

There's an 7-bit counter attached to a 16MHz clock. Every time a pulse comes
in, or the counter overflows, we sample it and send the result to the DMA
channel. The 8th bit contains the inverse of the actual pulse, so we can
distinguish rollovers (which show up as 0xff) from actual data.

An HD floppy has a nominal clock of 500kHz, so we use a sample clock of 8MHz
(every 0.125us). This means that our 500kHz pulses will have an interval of
16 (and a DD disk with a 250kHz nominal clock has an interval of 32). This
gives us more than enough resolution. If no pulse comes in, then we sample on
rollover at 16us.

VERY IMPORTANT:

The READ DATA line on the PSOC is on pin 3[2], which has a integrated
capacitor! This needs to be removed before the device can see any floppy
drive data. It's C7, and is easy with some tweezers and a stead hand with
a soldering iron.

Some useful numbers:

  - nominal rotation speed is 300 rpm, or 5Hz. The period is 200ms.
  - MFM encoding uses a clock of 500kHz. This makes each recording cell 2us.
  - a pulse is 150ns to 800ns, so a clock of 7MHz will sample it.
  
Useful links:

http://www.hermannseib.com/documents/floppy.pdf

https://hxc2001.com/download/datasheet/floppy/thirdparty/Teac/TEAC%20FD-05HF-8830.pdf
