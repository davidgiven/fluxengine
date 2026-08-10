package com.cowlark.fluxengine.decoders;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.external.FmMfm;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FluxDecoderTest
{
    private static final int CLOCK_TICKS = 1000;
    private static final double CLOCK_NS = CLOCK_TICKS * 1000000000.0 / 12000000.0;

    private static Bytes roundTrip(Bytes data)
    {
        /* Encode the data as an MFM bitstream... */
        Bits encoded = FmMfm.encodeMfm(data, new boolean[1]).toBits();

        /* ...write it out as flux... */
        Fluxmap map = new Fluxmap();
        map.appendBits(encoded, CLOCK_NS);
        FluxmapReader reader = new FluxmapReader(map, DecoderProto.getDefaultInstance());
        FluxDecoder decoder = new FluxDecoder(reader, CLOCK_NS, DecoderProto.getDefaultInstance());

        /* ...and read the raw bits back, skipping the PLL init pulse. */
        Bits decoded = new Bits();
        decoder.readBit();
        while (!reader.eof())
            decoded.add(decoder.readBit());

        return FmMfm.decodeFmMfm(decoded);
    }

    @Test
    public void roundTripsMfmData()
    {
        Bytes data = Bytes.of(0x81, 0x00, 0xa1, 0x4e, 0x4e);
        assertThat(roundTrip(data)).isEqualTo(data);
    }

    @Test
    public void emitsAClockForEveryFluxTransition()
    {
        /* A pulse at every cell boundary reads back as an unbroken run of
         * trues. */
        Fluxmap map = new Fluxmap();
        map.appendBits(
                java.util.Arrays.asList(true, true, true, true, true, true, true, true),
                CLOCK_NS);
        FluxmapReader reader = new FluxmapReader(map, DecoderProto.getDefaultInstance());
        FluxDecoder decoder = new FluxDecoder(reader, CLOCK_NS, DecoderProto.getDefaultInstance());

        Bits bits = new Bits();
        while (!reader.eof())
            bits.add(decoder.readBit());

        assertThat(bits.size()).isEqualTo(9);
        for (int i = 0; i < bits.size(); i++)
            assertThat(bits.getBit(i)).isTrue();
    }

    @Test
    public void firstBitIsAlwaysTrue()
    {
        /* The initial leading-zeroes state (tell().zeroes() == 0) makes the
         * first readBit return true. */
        Fluxmap map = new Fluxmap();
        map.appendBits(java.util.Arrays.asList(true), CLOCK_NS);
        FluxmapReader reader = new FluxmapReader(map, DecoderProto.getDefaultInstance());
        FluxDecoder decoder = new FluxDecoder(reader, CLOCK_NS, DecoderProto.getDefaultInstance());

        assertThat(decoder.readBit()).isTrue();
    }
}