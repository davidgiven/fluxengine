#include <libusbp_internal.h>

static libusbp_error * error_invalid_pipe_id(uint8_t pipe_id)
{
    return error_create("Invalid pipe ID 0x%02x.", pipe_id);
}

libusbp_error * check_pipe_id(uint8_t pipe_id)
{
    if ((pipe_id & ~0x80) > MAX_ENDPOINT_NUMBER || pipe_id == 0x80)
    {
        return error_invalid_pipe_id(pipe_id);
    }
    return NULL;
}

libusbp_error * check_pipe_id_in(uint8_t pipe_id)
{
    if (!(pipe_id & 0x80))
    {
        return error_invalid_pipe_id(pipe_id);
    }
    return check_pipe_id(pipe_id);
}

libusbp_error * check_pipe_id_out(uint8_t pipe_id)
{
    if (pipe_id & 0x80)
    {
        return error_invalid_pipe_id(pipe_id);
    }
    return check_pipe_id(pipe_id);
}
