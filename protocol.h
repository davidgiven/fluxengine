#ifndef PROTOCOL_H
#define PROTOCOL_H

enum 
{
    FLUXENGINE_VERSION = 9,

    FLUXENGINE_VID = 0x1209,
    FLUXENGINE_PID = 0x6e00,

    /* libusb uses these numbers */
    FLUXENGINE_DATA_OUT_EP = 0x01,
    FLUXENGINE_DATA_IN_EP = 0x82,
    FLUXENGINE_CMD_OUT_EP = 0x03,
    FLUXENGINE_CMD_IN_EP = 0x84,

    /* the PSoC code uses these, sigh */
    FLUXENGINE_DATA_OUT_EP_NUM = FLUXENGINE_DATA_OUT_EP & 0x0f,
    FLUXENGINE_DATA_IN_EP_NUM = FLUXENGINE_DATA_IN_EP & 0x0f,
    FLUXENGINE_CMD_OUT_EP_NUM = FLUXENGINE_CMD_OUT_EP & 0x0f,
    FLUXENGINE_CMD_IN_EP_NUM = FLUXENGINE_CMD_IN_EP & 0x0f,
    
    SIDE_SIDEA = 0<<0,
    SIDE_SIDEB = 1<<0,
    
    DRIVE_0 = 0,
    DRIVE_1 = 1,
    DRIVE_DD = 0<<1,
    DRIVE_HD = 1<<1,
    
    FRAME_SIZE = 64,
    TICK_FREQUENCY = 12000000,
    TICKS_PER_US = TICK_FREQUENCY / 1000000,
    TICKS_PER_MS = TICK_FREQUENCY / 1000,

    PRECOMPENSATION_THRESHOLD_TICKS = (int)(2.25 * TICKS_PER_US),
};

#define NS_PER_TICK (1000000000.0 / (double)TICK_FREQUENCY)
#define US_PER_TICK (1000000.0 / (double)TICK_FREQUENCY)
#define MS_PER_TICK (1000.0 / (double)TICK_FREQUENCY)

enum
{
    F_FRAME_ERROR = 0,            /* any_frame */
    F_FRAME_DEBUG,                /* debug_frame */
    F_FRAME_GET_VERSION_CMD,      /* any_frame */
    F_FRAME_GET_VERSION_REPLY,    /* version_frame */
    F_FRAME_SEEK_CMD,             /* seek_frame */
    F_FRAME_SEEK_REPLY,           /* any_frame */
    F_FRAME_MEASURE_SPEED_CMD,    /* any_frame */
    F_FRAME_MEASURE_SPEED_REPLY,  /* speed_frame */
    F_FRAME_BULK_TEST_CMD,        /* any_frame */
    F_FRAME_BULK_TEST_REPLY,      /* any_frame */
    F_FRAME_READ_CMD,             /* read_frame */
    F_FRAME_READ_REPLY,           /* any_frame */
    F_FRAME_READ_QD_CMD,          /* read_frame */
    F_FRAME_READ_QD_REPLY,        /* any_frame */
    F_FRAME_WRITE_CMD,            /* write_frame */
    F_FRAME_WRITE_REPLY,          /* any_frame */
    F_FRAME_ERASE_CMD,            /* erase_frame */
    F_FRAME_ERASE_REPLY,          /* any_frame */
    F_FRAME_RECALIBRATE_CMD,      /* any_frame */
    F_FRAME_RECALIBRATE_REPLY,    /* any_frame */
    F_FRAME_SET_DRIVE_CMD,        /* setdrive_frame */
    F_FRAME_SET_DRIVE_REPLY,      /* any_frame */
};

enum
{
    F_ERROR_NONE = 0,
    F_ERROR_BAD_COMMAND,
    F_ERROR_UNDERRUN,
    F_ERROR_INVALID_VALUE,
    F_ERROR_INTERNAL,
};

enum
{
    F_OP_PULSE = 0x80,
    F_OP_INDEX = 0x81
};

struct frame_header
{
    uint8_t type;
    uint8_t size;
};

struct any_frame
{
    struct frame_header f;
};

struct error_frame
{
    struct frame_header f;
    uint8_t error;
};

struct debug_frame
{
    struct frame_header f;
    char payload[60];
};

struct version_frame
{
    struct frame_header f;
    uint8_t version;
};

struct seek_frame
{
    struct frame_header f;
    uint8_t track;
};

struct speed_frame
{
    struct frame_header f;
    uint16_t period_ms;
};

struct read_frame
{
    struct frame_header f;
    uint8_t side;
    uint8_t revolutions;
};

struct write_frame
{
    struct frame_header f;
    uint8_t side;
    uint32_t bytes_to_write;
};

struct erase_frame
{
    struct frame_header f;
    uint8_t side;
};

struct set_drive_frame
{
    struct frame_header f;
    uint8_t drive_flags;
};

#endif
