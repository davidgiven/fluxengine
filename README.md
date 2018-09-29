The sampling system is dumb as rocks.

There's an 8-bit down counter attached to a clock. Every time a pulse comes
in, or the counter overflows, we sample the clock and send the result to the
DMA channel. This gives us a sequence of bytes which indicating the timing
between pulses.

An HD floppy has a nominal clock of 500kHz, so we use a sample clock of 8MHz
(every 0.125us). This means that our 500kHz pulses will have an interval of
16 (and a DD disk with a 250kHz nominal clock has an interval of 32). This
gives us more than enough resolution. If no pulse comes in, then we sample on
rollover at 256 * 0.125us = 320us. We distinguish between having a pulse and
not having a pulse by artifically nudging pulses on the 0 interval up to a 1,
so we know that a byte of 0 always means no pulse.

Useful links:

http://www.hermannseib.com/documents/floppy.pdf

https://hxc2001.com/download/datasheet/floppy/thirdparty/Teac/TEAC%20FD-05HF-8830.pdf
