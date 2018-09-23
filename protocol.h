#ifndef PROTOCOL_H
#define PROTOCOL_H

#define BUFFER_SIZE 61

enum 
{
    FLUXENGINE_ID = 'F'
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
