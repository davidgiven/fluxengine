package com.cowlark.fluxengine.external;

import org.indunet.fastproto.ByteOrder;
import org.indunet.fastproto.annotation.BinaryType;
import org.indunet.fastproto.annotation.Expect;
import org.indunet.fastproto.annotation.StringType;
import org.indunet.fastproto.annotation.UInt16Type;
import org.indunet.fastproto.annotation.UInt32ArrayType;
import org.indunet.fastproto.annotation.UInt32Type;
import org.indunet.fastproto.annotation.UInt8Type;

/* A2R 3.x structures from https://applesaucefdc.com/a2r/. All offsets are relative to
 * the structure being decoded, and all multibyte integers are little-endian.
 * Chunk bodies exclude ChunkHeader. Capture and track entries exclude the final EndMarker;
 * callers must inspect the next mark before decoding an entry. Flux bytes remain packed,
 * with 255 extending a timing value. Metadata remains tab-delimited UTF-8 text. */
public final class A2R3
{
    public static final int CHUNK_INFO = 0x4f464e49;
    public static final int CHUNK_RWCP = 0x50435752;
    public static final int CHUNK_SLVD = 0x44564c53;
    public static final int CHUNK_META = 0x4154454d;
    public static final int INFO_CHUNK_VERSION = 1;
    public static final int RWCP_CHUNK_VERSION = 1;
    public static final int SLVD_CHUNK_VERSION = 2;
    public static final int DEFAULT_PS_PER_TICK = 62500;

    public static final int DRIVE_525_SS_40_QUARTER_STEP = 1;
    public static final int DRIVE_35_DS_80_APPLE_CLV = 2;
    public static final int DRIVE_525_DS_80 = 3;
    public static final int DRIVE_525_DS_40 = 4;
    public static final int DRIVE_35_DS_80 = 5;
    public static final int DRIVE_8_DS = 6;
    public static final int DRIVE_3_DS_80 = 7;
    public static final int DRIVE_3_DS_40 = 8;

    public static final int TIMING = 1;
    public static final int BITS = 2;
    public static final int XTIMING = 3;
    public static final int CAPTURE_MARK = 0x43;
    public static final int TRACK_MARK = 0x54;
    public static final int END_MARK = 0x58;
    public static final byte[] FILEHEADER =
            {'A', '2', 'R', '3', (byte) 0xff, 0x0a, 0x0d, 0x0a};

    private A2R3()
    {
    }

    public static class FileHeader
    {
        @Expect(offset = 0, bytes = {'A', '2', 'R', '3', (byte) 0xff, 0x0a, 0x0d, 0x0a})
        transient int _expects;
    }

    public static class ChunkHeader
    {
        @UInt32Type(offset = 0, byteOrder = ByteOrder.LITTLE) public long id;
        @UInt32Type(offset = 4, byteOrder = ByteOrder.LITTLE) public long size;
    }

    /* The creator is exactly 32 UTF-8 bytes, padded with spaces by the writer. */
    public static class Info
    {
        @Expect(offset = 0, bytes = {INFO_CHUNK_VERSION}) transient int _expects;
        @StringType(offset = 1, length = 32, charset = "UTF-8")
        public String creator = "                                ";
        @UInt8Type(offset = 33) public int driveType;
        @UInt8Type(offset = 34) public int writeProtected;
        @UInt8Type(offset = 35) public int synchronizedCapture;
        @UInt8Type(offset = 36) public int hardSectorCount;
    }

    /* The entries start at offset 16 in the chunk body. Reserved bytes must be zero. */
    public static class RawCaptureHeader
    {
        @Expect(offset = 0, bytes = {RWCP_CHUNK_VERSION}) transient int _expects;
        @UInt32Type(offset = 1, byteOrder = ByteOrder.LITTLE)
        public long resolution = DEFAULT_PS_PER_TICK;
        @BinaryType(offset = 5, length = 11) public byte[] reserved = new byte[11];
    }

    public static class SolvedHeader
    {
        @Expect(offset = 0, bytes = {SLVD_CHUNK_VERSION}) transient int _expects;
        @UInt32Type(offset = 1, byteOrder = ByteOrder.LITTLE)
        public long resolution = DEFAULT_PS_PER_TICK;
        @BinaryType(offset = 5, length = 11) public byte[] reserved = new byte[11];
    }

    /* Index signals are absolute tick offsets. Location is a quarter track for drive type 1,
     * or (cylinder << 1) + side for all other drive types. Counts and sizes must match arrays. */
    public static class Capture
    {
        @Expect(offset = 0, bytes = {CAPTURE_MARK}) transient int _expects;
        @UInt8Type(offset = 1) public int captureType;
        @UInt16Type(offset = 2, byteOrder = ByteOrder.LITTLE) public int location;
        @UInt8Type(offset = 4) public int indexSignalCount;
        @UInt32ArrayType(offset = 5, lengthRef = "$indexSignalCount", byteOrder = ByteOrder.LITTLE)
        public long[] indexSignals = new long[0];
    }

    public static class Track
    {
        @Expect(offset = 0, bytes = {TRACK_MARK}) transient int _expects;
        @UInt16Type(offset = 1, byteOrder = ByteOrder.LITTLE) public int location;
        @UInt8Type(offset = 3) public int mirrorDistanceOutward;
        @UInt8Type(offset = 4) public int mirrorDistanceInward;
        @BinaryType(offset = 5, length = 6) public byte[] reserved = new byte[6];
        @UInt8Type(offset = 11) public int indexSignalCount;
        @UInt32ArrayType(offset = 12, lengthRef = "$indexSignalCount", byteOrder = ByteOrder.LITTLE)
        public long[] indexSignals = new long[0];
    }

    /* Follows the variable index array in either Capture or Track. FastProto lengthRef
     * references fields directly, so the variable-position tail is a separate structure. */
    public static class FluxData
    {
        @UInt32Type(offset = 0, byteOrder = ByteOrder.LITTLE) public long dataSize;
        @BinaryType(offset = 4, lengthRef = "$dataSize") public byte[] data = new byte[0];
    }

    public static class EndMarker
    {
        @Expect(offset = 0, bytes = {END_MARK}) transient int _expects;
    }

    /* Includes the META chunk header so the UTF-8 byte length is available to FastProto. */
    public static class Metadata
    {
        @Expect(offset = 0, bytes = {'M', 'E', 'T', 'A'}) transient int _expects;
        @UInt32Type(offset = 4, byteOrder = ByteOrder.LITTLE) public long size;
        @StringType(offset = 8, lengthRef = "$size", charset = "UTF-8") public String text = "";
    }
}
