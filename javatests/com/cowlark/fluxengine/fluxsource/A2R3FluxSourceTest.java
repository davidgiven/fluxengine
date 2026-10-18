package com.cowlark.fluxengine.fluxsource;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class A2R3FluxSourceTest
{
    private static byte[] chunk(String id, byte[] data)
    {
        return ByteBuffer.allocate(data.length + 8).order(ByteOrder.LITTLE_ENDIAN)
                .put(id.getBytes(java.nio.charset.StandardCharsets.US_ASCII))
                .putInt(data.length).put(data).array();
    }

    private static byte[] captures(int resolution, int type, byte[] data)
    {
        return ByteBuffer.allocate(16 + 13 + data.length + 1).order(ByteOrder.LITTLE_ENDIAN)
                .put((byte) 1).putInt(resolution).put(new byte[11])
                .put((byte) 'C').put((byte) type).putShort((short) 3).put((byte) 1)
                .putInt(32).putInt(data.length).put(data).put((byte) 'X').array();
    }

    private static FluxSource source(byte[]... chunks) throws Exception
    {
        ByteArrayOutputStream file = new ByteArrayOutputStream();
        file.write(new byte[] {'A', '2', 'R', '3', (byte) 255, 10, 13, 10});
        byte[] info = new byte[37];
        info[0] = 1;
        java.util.Arrays.fill(info, 1, 33, (byte) ' ');
        info[33] = 5;
        file.write(chunk("INFO", info));
        for (byte[] bytes : chunks)
            file.write(bytes);
        Path path = Files.createTempFile("a2r3", ".a2r");
        try
        {
            Files.write(path, file.toByteArray());
            FluxSource source = A2RFluxSource.create(
                    A2rFluxSourceProto.newBuilder().setFilename(path.toString()).build());
            assertTrue(source instanceof A2R3FluxSource);
            return source;
        } finally
        {
            Files.delete(path);
        }
    }

    private static FluxSourceIterator track(FluxSource source)
    {
        return source.readFlux(FluxReadParameters.builder().setCylinder(1).setHead(1).build());
    }

    @Test
    public void readsArbitraryTimingLengthsAcrossChunks() throws Exception
    {
        FluxSource source = source(chunk("RWCP", captures(62500, 1, new byte[] {64, 64})),
                chunk("SLVD", new byte[] {0}), chunk("META", new byte[] {0}),
                chunk("RWCP", captures(125000, 3, new byte[] {(byte) 255, 10, 1})));
        FluxSourceIterator iterator = track(source);
        Fluxmap first = iterator.next();
        assertEquals(8000, first.durationNs(), 50);
        assertEquals(1, first.getIndexMarks().size());
        assertEquals(2000, first.getIndexMarks().first(), 50);
        Fluxmap second = iterator.next();
        assertEquals(33250, second.durationNs(), 50);
        assertFalse(iterator.hasNext());
        assertTrue(track(source).hasNext());
        assertFalse(source.readFlux(FluxReadParameters.builder().setCylinder(0).setHead(0).build())
                .hasNext());
        assertEquals("c1h1", source.getExtraConfig().getDrive().getTracks());
    }

    @Test
    public void readsBitsMsbFirstAndPreservesTrailingZeros() throws Exception
    {
        Fluxmap map = track(source(chunk("RWCP", captures(62500, 2, new byte[] {(byte) 0x80}))))
                .next();
        assertEquals(32000, map.durationNs(), 50);
        int pulseCount = 0;
        long ticks = 0;
        for (byte value : map.rawBytes().toByteArray())
        {
            ticks += value & 0x3f;
            if ((value & 0x80) != 0)
            {
                pulseCount++;
                assertEquals(4000, ticks * com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK, 50);
            }
        }
        assertEquals(1, pulseCount);
    }

    @Test
    public void rejectsTruncatedChunksAndMissingTerminator()
    {
        assertThrows(FluxEngineException.class,
                () -> source(new byte[] {'R', 'W', 'C', 'P', -1, -1, -1, 127}));
        byte[] body = captures(62500, 1, new byte[] {64});
        assertThrows(FluxEngineException.class,
                () -> source(chunk("RWCP", java.util.Arrays.copyOf(body, body.length - 1))));
    }
}
