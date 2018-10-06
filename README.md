The sampling system is dumb as rocks.

There's an 8-bit counter attached to an 12MHz clock. This is used to measure
the interval between pulses. If the timer overflows, we pretend it's a pulse
(this very rarely happens in real life).

An HD floppy has a nominal clock of 500kHz, so we use a sample clock of 12MHz
(every 83ns). This means that our 500kHz pulses will have an interval of 24
(and a DD disk with a 250kHz nominal clock has an interval of 48). This gives
us more than enough resolution. If no pulse comes in, then we sample on
rollover at 21us.

(The clock needs to be absolutely rock solid or we get jitter which makes the
data difficult to analyse, so 12 was chosen to be derivable from the
ultra-accurate USB clock.)

VERY IMPORTANT:

Some of the pins on the PSoC have integrated capacitors, which will play
havoc with your data! These are C7, C9, C12 and C13. If you don't, your
floppy drive will be almost unusable (the motor will run but you won't see
any data). They're easy enough with some tweezers and a steady hand with a
soldering iron.

Some useful numbers:

  - nominal rotation speed is 300 rpm, or 5Hz. The period is 200ms.
  - MFM encoding uses a clock of 500kHz. This makes each recording cell 2us.
  - a pulse is 150ns to 800ns, so a clock of 7MHz will sample it.
  
Useful links:

http://www.hermannseib.com/documents/floppy.pdf

https://hxc2001.com/download/datasheet/floppy/thirdparty/Teac/TEAC%20FD-05HF-8830.pdf
