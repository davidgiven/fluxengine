package com.cowlark.fluxengine.wiring;

/**
 * Wire protocol definitions for the FluxEngine hardware.
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

    public static class FrameHeader
    {
        public int type;
        public int size;
    }

    public static class AnyFrame
    {
        public FrameHeader f;
    }

    public static class ErrorFrame
    {
        public FrameHeader f;
        public int error;
    }

    public static class DebugFrame
    {
        public FrameHeader f;
        public byte[] payload = new byte[60];
    }

    public static class VersionFrame
    {
        public FrameHeader f;
        public int version;
    }

    public static class SeekFrame
    {
        public FrameHeader f;
        public int track;
    }

    public static class MeasureSpeedFrame
    {
        public FrameHeader f;
        public int hardSectorCount;
    }

    public static class SpeedFrame
    {
        public FrameHeader f;
        public int periodMs;
    }

    public static class ReadFrame
    {
        public FrameHeader f;
        public int side;
        public int synced;
        public int milliseconds;
        public int hardsecThresholdMs;
    }

    public static class WriteFrame
    {
        public FrameHeader f;
        public int side;
        public long bytesToWrite;
        public int hardsecThresholdMs;
    }

    public static class EraseFrame
    {
        public FrameHeader f;
        public int side;
        public int hardsecThresholdMs;
    }

    public static class SetDriveFrame
    {
        public FrameHeader f;
        public int drive;
        public int highDensity;
        public int indexMode;
    }

    public static class Voltages
    {
        public int logic0Mv;
        public int logic1Mv;
    }

    public static class VoltagesFrame
    {
        public FrameHeader f;
        public Voltages outputBothOff = new Voltages();
        public Voltages outputDrive0Selected = new Voltages();
        public Voltages outputDrive1Selected = new Voltages();
        public Voltages outputDrive0Running = new Voltages();
        public Voltages outputDrive1Running = new Voltages();
        public Voltages inputBothOff = new Voltages();
        public Voltages inputDrive0Selected = new Voltages();
        public Voltages inputDrive1Selected = new Voltages();
        public Voltages inputDrive0Running = new Voltages();
        public Voltages inputDrive1Running = new Voltages();
    }
}
