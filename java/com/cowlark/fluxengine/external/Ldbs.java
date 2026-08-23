package com.cowlark.fluxengine.external;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.FluxEngineException;
import java.util.Map;
import java.util.TreeMap;

/**
 * A very simple interface to John Elliott's LDBS image format:
 * http://www.seasip.info/Unix/LibDsk/ldbs.html
 * ported from lib/external/ldbs.{h,cc}.
 */
public class Ldbs
{
    public static final int FILE_MAGIC = 0x4c425301;
    public static final int FILE_TYPE = 0x44534b02;
    public static final int BLOCK_MAGIC = 0x4c444201;
    public static final int TRACK_BLOCK = 0x44495201;

    private static final class Block
    {
        int type;
        Bytes data = new Bytes();
    }

    private final TreeMap<Integer, Block> blocks = new TreeMap<>();
    private int top = 20;

    public Bytes get(int address)
    {
        Block block = blocks.get(address);
        if (block == null)
            throw new FluxEngineException("no such LDBS block");
        return block.data;
    }

    public int put(Bytes data, int type)
    {
        int address = top;
        Block block = new Block();
        block.type = type;
        block.data = data;

        blocks.put(address, block);

        top += data.size() + 20;
        return address;
    }

    public int read(Bytes data)
    {
        ByteReader br = new ByteReader(data);

        br.seek(0);
        if ((br.readBe32() != FILE_MAGIC) || (br.readBe32() != FILE_TYPE))
            throw new FluxEngineException("not a valid LDBS file");

        int address = br.readLe32();
        br.skip(4);
        int trackDirectory = br.readLe32();

        while (address != 0)
        {
            br.seek(address);
            if (br.readBe32() != BLOCK_MAGIC)
                throw new FluxEngineException(
                        String.format("invalid block at address 0x%x", address));

            int blockAddress = address;
            Block block = new Block();
            block.type = br.readBe32();

            int size = br.readLe32();
            br.skip(4);
            address = br.readLe32();

            block.data = br.read(size);
            blocks.put(blockAddress, block);
        }

        top = data.size();
        return trackDirectory;
    }

    public Bytes write(int trackDirectory)
    {
        Bytes data = new Bytes(top);
        ByteWriter bw = new ByteWriter(data);

        int previous = 0;
        for (Map.Entry<Integer, Block> e : blocks.entrySet())
        {
            bw.seek(e.getKey());
            bw.writeBe32(BLOCK_MAGIC);
            bw.writeBe32(e.getValue().type);
            bw.writeLe32(e.getValue().data.size());
            bw.writeLe32(e.getValue().data.size());
            bw.writeLe32(previous);
            bw.write(e.getValue().data);

            previous = e.getKey();
        }

        bw.seek(0);
        bw.writeBe32(FILE_MAGIC);
        bw.writeBe32(FILE_TYPE);
        bw.writeLe32(previous);
        bw.writeLe32(0);
        bw.writeLe32(trackDirectory);

        return data;
    }
}
