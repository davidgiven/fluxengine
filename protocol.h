#ifndef PROTOCOL_H
#define PROTOCOL_H

enum 
{
    FLUXENGINE_ID = 'F',

    FLUXENGINE_VID = 0xF055,
    FLUXENGINE_PID = 0x9971
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
