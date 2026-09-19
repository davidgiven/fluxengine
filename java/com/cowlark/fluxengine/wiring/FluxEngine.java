package com.cowlark.fluxengine.wiring;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.NoArgsConstructor;
import org.indunet.fastproto.annotation.BinaryType;
import org.indunet.fastproto.annotation.Expect;
import org.indunet.fastproto.annotation.UInt16Type;
import org.indunet.fastproto.annotation.UInt32Type;
import org.indunet.fastproto.annotation.UInt8Type;

/**
 * Wire protocol definitions for the FluxEngine hardware, ported from protocol.h.
 *
 * <p>Each inner class corresponds to a struct in protocol.h and is annotated for
 * use with FastProto. See RolandFilesystem.java for an example of FastProto usage.
 */
public final class FluxEngine
{
    public static final int FLUXENGINE_PROTOCOL_VERSION = 17;

    public static final int FLUXENGINE_VID = 0x1209;
    public static final int FLUXENGINE_PID = 0x6e00;
    public static final int FLUXENGINE_ID = (FLUXENGINE_VID << 16) | FLUXENGINE_PID;

    /* libusb uses these numbers */
    public static final int FLUXENGINE_DATA_OUT_EP = 0x01;
    public static final int FLUXENGINE_DATA_IN_EP = 0x82;
    public static final int FLUXENGINE_CMD_OUT_EP = 0x03;
    public static final int FLUXENGINE_CMD_IN_EP = 0x84;

    /* the PSoC code uses these, sigh */
    public static final int FLUXENGINE_DATA_OUT_EP_NUM = FLUXENGINE_DATA_OUT_EP & 0x0f;
    public static final int FLUXENGINE_DATA_IN_EP_NUM = FLUXENGINE_DATA_IN_EP & 0x0f;
    public static final int FLUXENGINE_CMD_OUT_EP_NUM = FLUXENGINE_CMD_OUT_EP & 0x0f;
    public static final int FLUXENGINE_CMD_IN_EP_NUM = FLUXENGINE_CMD_IN_EP & 0x0f;

    public static final int SIDE_SIDEA = 0 << 0;
    public static final int SIDE_SIDEB = 1 << 0;

    public static final int DRIVE_0 = 0;
    public static final int DRIVE_1 = 1;
    public static final int DRIVE_DD = 0 << 1;
    public static final int DRIVE_HD = 1 << 1;

    public static final int FRAME_SIZE = 64;
    public static final int TICK_FREQUENCY = 12000000;
    public static final int TICKS_PER_US = TICK_FREQUENCY / 1000000;
    public static final int PRECOMPENSATION_THRESHOLD_TICKS = (int) (2.25 * TICKS_PER_US);
    public static final int TICKS_PER_MS = TICK_FREQUENCY / 1000;
    public static final double NS_PER_TICK = 1000000000.0 / TICK_FREQUENCY;
    public static final double US_PER_TICK = 1000000.0 / TICK_FREQUENCY;
    public static final double MS_PER_TICK = 1000.0 / TICK_FREQUENCY;

    public static final int F_FRAME_ERROR = 0;
    public static final int F_FRAME_DEBUG = 1;
    public static final int F_FRAME_GET_VERSION_CMD = 2;
    public static final int F_FRAME_GET_VERSION_REPLY = 3;
    public static final int F_FRAME_SEEK_CMD = 4;
    public static final int F_FRAME_SEEK_REPLY = 5;
    public static final int F_FRAME_MEASURE_SPEED_CMD = 6;
    public static final int F_FRAME_MEASURE_SPEED_REPLY = 7;
    public static final int F_FRAME_BULK_WRITE_TEST_CMD = 8;
    public static final int F_FRAME_BULK_WRITE_TEST_REPLY = 9;
    public static final int F_FRAME_BULK_READ_TEST_CMD = 10;
    public static final int F_FRAME_BULK_READ_TEST_REPLY = 11;
    public static final int F_FRAME_READ_CMD = 12;
    public static final int F_FRAME_READ_REPLY = 13;
    public static final int F_FRAME_WRITE_CMD = 14;
    public static final int F_FRAME_WRITE_REPLY = 15;
    public static final int F_FRAME_ERASE_CMD = 16;
    public static final int F_FRAME_ERASE_REPLY = 17;
    public static final int F_FRAME_RECALIBRATE_CMD = 18;
    public static final int F_FRAME_RECALIBRATE_REPLY = 19;
    public static final int F_FRAME_SET_DRIVE_CMD = 20;
    public static final int F_FRAME_SET_DRIVE_REPLY = 21;
    public static final int F_FRAME_MEASURE_VOLTAGES_CMD = 22;
    public static final int F_FRAME_MEASURE_VOLTAGES_REPLY = 23;

    public static final int F_ERROR_NONE = 0;
    public static final int F_ERROR_BAD_COMMAND = 1;
    public static final int F_ERROR_UNDERRUN = 2;
    public static final int F_ERROR_INVALID_VALUE = 3;
    public static final int F_ERROR_INTERNAL = 4;

    public static final int F_INDEX_REAL = 0;
    public static final int F_INDEX_300 = 1;
    public static final int F_INDEX_360 = 2;

    public static final int F_BIT_PULSE = 0x80;
    public static final int F_BIT_INDEX = 0x40;
    public static final int F_DESYNC = 0x00; /* obsolete */
    public static final int F_EOF = 0x100; /* synthetic, only produced by library */

    private FluxEngine()
    {
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class FrameHeader
    {
        @UInt8Type(offset = 0) public int type;
        @UInt8Type(offset = 1) public int size;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class AnyFrame
    {
        @UInt8Type(offset = 0) public int type;
        @UInt8Type(offset = 1) public int size;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class ErrorFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_ERROR, 3}) transient int _expects;
        @UInt8Type(offset = 2) public int error;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class DebugFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_DEBUG, 62}) transient int _expects;
        @BinaryType(offset = 2, length = 60) public byte[] payload;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class VersionFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_GET_VERSION_CMD, 2}) transient int _expects;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class VersionReplyFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_GET_VERSION_REPLY, 3}) transient int _expects;
        @UInt8Type(offset = 2) public int version;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class SeekFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_SEEK_CMD, 3}) transient int _expects;
        @UInt8Type(offset = 2) public int track;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class MeasureSpeedFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_MEASURE_SPEED_CMD, 3}) transient int _expects;
        @UInt8Type(offset = 2) public int hardSectorCount;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class MeasureSpeedReplyFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_MEASURE_SPEED_REPLY, 4}) transient int _expects;
        @UInt16Type(offset = 2) public int periodMs;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class ReadFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_READ_CMD, 7}) transient int _expects;
        @UInt8Type(offset = 2) public int side;
        @UInt8Type(offset = 3) public int synced;
        @UInt16Type(offset = 4) public int milliseconds;
        @UInt8Type(offset = 6) public int hardsecThresholdMs;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class WriteFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_WRITE_CMD, 8}) transient int _expects;
        @UInt8Type(offset = 2) public int side;
        @UInt32Type(offset = 3) public long bytesToWrite;
        @UInt8Type(offset = 7) public int hardsecThresholdMs;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class EraseFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_ERASE_CMD, 4}) transient int _expects;
        @UInt8Type(offset = 2) public int side;
        @UInt8Type(offset = 3) public int hardsecThresholdMs;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class SetDriveFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_SET_DRIVE_CMD, 5}) transient int _expects;
        @UInt8Type(offset = 2) public int drive;
        @UInt8Type(offset = 3) public int highDensity;
        @UInt8Type(offset = 4) public int indexMode;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class Voltages
    {
        @UInt16Type(offset = 0) public int logic0Mv;
        @UInt16Type(offset = 2) public int logic1Mv;
    }

    @Data
    @Builder(setterPrefix = "set")
    @NoArgsConstructor
    @AllArgsConstructor
    public static class VoltagesReplyFrame
    {
        @Expect(offset = 0, bytes = {(byte) F_FRAME_MEASURE_VOLTAGES_REPLY, 42}) transient int
                _expects;
        public Voltages outputBothOff;
        public Voltages outputDrive0Selected;
        public Voltages outputDrive1Selected;
        public Voltages outputDrive0Running;
        public Voltages outputDrive1Running;
        public Voltages inputBothOff;
        public Voltages inputDrive0Selected;
        public Voltages inputDrive1Selected;
        public Voltages inputDrive0Running;
        public Voltages inputDrive1Running;
    }
}
