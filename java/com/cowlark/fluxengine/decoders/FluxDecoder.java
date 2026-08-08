package com.cowlark.fluxengine.decoders;

import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.data.FluxPosition;
import com.cowlark.fluxengine.data.FluxmapReader;

/* This is a port of the samdisk code:
 *
 * https://github.com/simonowen/samdisk/blob/master/src/FluxDecoder.cpp
 *
 * I'm not actually terribly sure how it works, but it does, and much better
 * than my code.
 */
public class FluxDecoder
{
    private final FluxmapReader fmr;
    private final double pllPhase;
    private final double pllAdjust;
    private final double fluxScale;
    private final double clockCentreNs;
    private final double clockMinNs;
    private final double clockMaxNs;
    private double clockNs;
    private double fluxNs = 0.0;
    private int clockedZeroes = 0;
    private int goodbits = 0;
    private boolean index = false;
    private boolean syncLost = false;
    private int leadingZeroes;

    public FluxDecoder(FluxmapReader fmr, double bitcellNs, DecoderProto config)
    {
        this.fmr = fmr;
        pllPhase = config.getPllPhase();
        pllAdjust = config.getPllAdjust();
        fluxScale = config.getFluxScale();
        clockNs = bitcellNs;
        clockCentreNs = bitcellNs;
        clockMinNs = bitcellNs * (1.0 - pllAdjust);
        clockMaxNs = bitcellNs * (1.0 + pllAdjust);
        leadingZeroes = fmr.tell().zeroes();
    }

    private static double clampClock(double min, double value, double max)
    {
        if (value > max)
            return max;
        if (value < min)
            return min;
        return value;
    }

    public boolean readBit()
    {
        if (leadingZeroes > 0)
        {
            leadingZeroes--;
            return false;
        } else if (leadingZeroes == 0)
        {
            leadingZeroes--;
            return true;
        }

        while (!fmr.eof() && fluxNs < clockNs / 2.0)
        {
            fluxNs += nextFlux() * fluxScale;
            clockedZeroes = 0;
        }

        fluxNs -= clockNs;
        if (fluxNs >= clockNs / 2.0)
        {
            clockedZeroes++;
            goodbits++;
            return false;
        }

        /* PLL adjustment: change the clock frequency according to the phase
         * mismatch */
        if (clockedZeroes <= 3)
        {
            /* In sync: adjust base clock */

            clockNs += fluxNs * pllAdjust;
        } else
        {
            /* Out of sync: adjust the base clock back towards the centre */

            clockNs += (clockCentreNs - clockNs) * pllAdjust;

            /* We require 256 good bits before reporting another sync loss
             * event. */

            if (goodbits >= 256)
                syncLost = true;
            goodbits = 0;
        }

        /* Clamp the clock's adjustment range. */

        clockNs = clampClock(clockMinNs, clockNs, clockMaxNs);

        /* I'm not sure what this does, but the original comment is:
         * Authentic PLL: Do not snap the timing window to each flux
         * transition */

        fluxNs *= 1.0 - pllPhase;

        goodbits++;
        return true;
    }

    public Bits readBits(int count)
    {
        Bits result = new Bits();
        while (!fmr.eof() && count-- > 0)
            result.add(readBit());
        return result;
    }

    public Bits readBits(FluxPosition until)
    {
        Bits result = new Bits();
        while (!fmr.eof() && fmr.tell().bytes() < until.bytes())
            result.add(readBit());
        return result;
    }

    public Bits readBits()
    {
        return readBits(Integer.MAX_VALUE);
    }

    private double nextFlux()
    {
        long ticks = fmr.readInterval((long) (clockCentreNs / NS_PER_TICK));
        return ticks * NS_PER_TICK;
    }
}