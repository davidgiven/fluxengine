#ifndef PROTOCOL_H
#define PROTOCOL_H

enum 
{
    FLUXENGINE_ID = 'F',

    FLUXENGINE_VID = 0xF055,
    FLUXENGINE_PID = 0x9973,
    
    FLUXENGINE_DATA_OUT_EP = 0x01,
    FLUXENGINE_DATA_IN_EP = 0x82,
    FLUXENGINE_CMD_OUT_EP = 0x03,
    FLUXENGINE_CMD_IN_EP = 0x84,
};

enum
{
    FRAME_OK = 0,
    FRAME_FINISHED,
    FRAME_COMMAND,
};

enum
{
    ERROR_NONE = 0,
    ERROR_UNDERRUN
};

#define BUFFER_SIZE 62
typedef struct
{
    uint8_t id;
    uint8_t type;
    union
    {
        uint8_t buffer[BUFFER_SIZE];
        uint8_t status;
    } u;
}
frame_t;

#endif
