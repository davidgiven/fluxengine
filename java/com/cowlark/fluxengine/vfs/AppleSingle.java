package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;

/**
 * AppleSingle container, ported from {@code old_cpp_version/lib/vfs/applesingle.cc}.
 * Holds a data fork, resource fork, and Finder metadata (type/creator).
 */
public class AppleSingle
{
    public static final int OVERHEAD = 0x5e;

    private static final int APPLESINGLE_MAGIC = 0x00051600;
    private static final int APPLESINGLE_VERSION = 0x00020000;
    public Bytes data = new Bytes();
    public Bytes rsrc = new Bytes();
    public Bytes type = new Bytes();
    public Bytes creator = new Bytes();

    public void parse(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        if (br.readBe32() != APPLESINGLE_MAGIC)
            throw new InvalidFileException("bad AppleSingle magic");
        if (Integer.compareUnsigned(br.readBe32(), APPLESINGLE_VERSION) > 0)
            throw new InvalidFileException("unsupported AppleSingle version");

        br.skip(16);
        int entries = br.readBe16();
        for (int i = 0; i < entries; i++)
        {
            int entryType = br.readBe32();
            int offset = br.readBe32();
            int length = br.readBe32();

            switch (entryType)
            {
                case 1:
                    data = bytes.slice(offset, length);
                    break;

                case 2:
                    rsrc = bytes.slice(offset, length);
                    break;

                case 9:
                {
                    Bytes finderInfo = bytes.slice(offset, length);
                    type = finderInfo.slice(0, 4);
                    creator = finderInfo.slice(4, 4);
                    break;
                }

                default:
                    break;
            }
        }
    }

    public Bytes render()
    {
        Bytes result = new Bytes();
        ByteWriter bw = new ByteWriter(result);

        bw.writeBe32(APPLESINGLE_MAGIC);
        bw.writeBe32(APPLESINGLE_VERSION);
        bw.pad(16);
        bw.writeBe16(3);

        bw.writeBe32(9);
        bw.writeBe32(0x3e);
        bw.writeBe32(32);

        bw.writeBe32(1);
        bw.writeBe32(OVERHEAD);
        bw.writeBe32(data.size());

        bw.writeBe32(2);
        bw.writeBe32(OVERHEAD + data.size());
        bw.writeBe32(rsrc.size());

        bw.write(type.slice(0, 4));
        bw.write(creator.slice(0, 4));
        bw.pad(32 - 8);

        bw.write(data);
        bw.write(rsrc);

        return result;
    }

    public static class InvalidFileException extends IllegalArgumentException
    {
        public InvalidFileException()
        {
            super("invalid AppleSingle file");
        }

        public InvalidFileException(String message)
        {
            super(message);
        }
    }
}
