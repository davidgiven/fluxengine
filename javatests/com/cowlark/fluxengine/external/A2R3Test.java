package com.cowlark.fluxengine.external;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import java.util.Arrays;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import org.indunet.fastproto.FastProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class A2R3Test
{
    @Test
    public void captureWithUnsignedIndexSignals()
    {
        byte[] bytes = ByteBuffer.allocate(20).order(ByteOrder.LITTLE_ENDIAN)
                .put((byte) 'C').put((byte) 3).putShort((short) 300).put((byte) 2)
                .putInt(0x80000000).putInt(0xffffffff).putInt(3)
                .put(new byte[] {(byte) 255, (byte) 255, 10}).array();
        A2R3.Capture capture = FastProto.decode(bytes, A2R3.Capture.class);
        assertEquals(300, capture.location);
        assertArrayEquals(new long[] {0x80000000L, 0xffffffffL}, capture.indexSignals);
        A2R3.FluxData data = FastProto.decode(Arrays.copyOfRange(bytes, 13, 20), A2R3.FluxData.class);
        assertArrayEquals(new byte[] {(byte) 255, (byte) 255, 10}, data.data);
        assertArrayEquals(Arrays.copyOf(bytes, 13), FastProto.encode(capture));
        assertArrayEquals(Arrays.copyOfRange(bytes, 13, 20), FastProto.encode(data));
    }

    @Test
    public void trackWithoutIndexSignals()
    {
        byte[] bytes = ByteBuffer.allocate(18).order(ByteOrder.LITTLE_ENDIAN)
                .put((byte) 'T').putShort((short) 12).put((byte) 1).put((byte) 2)
                .put(new byte[6]).put((byte) 0).putInt(2).put(new byte[] {64, 65}).array();
        A2R3.Track track = FastProto.decode(bytes, A2R3.Track.class);
        assertEquals(12, track.location);
        assertEquals(0, track.indexSignals.length);
        A2R3.FluxData data = FastProto.decode(Arrays.copyOfRange(bytes, 12, 18), A2R3.FluxData.class);
        assertArrayEquals(new byte[] {64, 65}, data.data);
        assertArrayEquals(Arrays.copyOf(bytes, 12), FastProto.encode(track));
        assertArrayEquals(Arrays.copyOfRange(bytes, 12, 18), FastProto.encode(data));
    }

    @Test
    public void fixedHeadersAndMetadata()
    {
        assertArrayEquals(A2R3.FILEHEADER, FastProto.encode(new A2R3.FileHeader()));
        assertArrayEquals(new byte[] {'X'}, FastProto.encode(new A2R3.EndMarker()));
        assertEquals(37, FastProto.encode(new A2R3.Info()).length);
        byte[] rawHeader = FastProto.encode(new A2R3.RawCaptureHeader());
        assertEquals(16, rawHeader.length);
        assertEquals(62500, ByteBuffer.wrap(rawHeader).order(ByteOrder.LITTLE_ENDIAN).getInt(1));
        assertEquals(2, FastProto.encode(new A2R3.SolvedHeader())[0]);
        byte[] metadata = {'M', 'E', 'T', 'A', 9, 0, 0, 0, 't', 'i', 't', 'l', 'e', 9,
                (byte) 0xc3, (byte) 0xa9, 10};
        A2R3.Metadata decoded = FastProto.decode(metadata, A2R3.Metadata.class);
        assertEquals("title\té\n", decoded.text);
        assertArrayEquals(metadata, FastProto.encode(decoded));
    }
}
