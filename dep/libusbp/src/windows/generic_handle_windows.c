#include <libusbp_internal.h>

// TODO: add a test for the issue where WinUSB cannot send data from read-only
// memory, then add a note about it to the documentation for
// libusbp_control_transfer and PLATFORM_NOTES.md.

struct libusbp_generic_handle
{
    HANDLE file_handle;
    WINUSB_INTERFACE_HANDLE winusb_handle;
};

libusbp_error * libusbp_generic_handle_open(
    const libusbp_generic_interface * gi,
    libusbp_generic_handle ** gh
)
{
    if (gh == NULL)
    {
        return error_create("Generic handle output pointer is null.");
    }

    *gh = NULL;

    if (gi == NULL)
    {
        return error_create("Generic interface is null.");
    }

    libusbp_error * error = NULL;

    // Allocate memory for the struct.
    libusbp_generic_handle * new_gh = malloc(sizeof(libusbp_generic_handle));
    if (new_gh == NULL)
    {
        error = &error_no_memory;
    }

    // Initialize the handles so we can clean up properly in case an error
    // happens.
    if (error == NULL)
    {
        new_gh->file_handle = INVALID_HANDLE_VALUE;
        new_gh->winusb_handle = INVALID_HANDLE_VALUE;
    }

    // Get the filename.
    char * filename = NULL;
    if (error == NULL)
    {
        error = libusbp_generic_interface_get_os_filename(gi, &filename);
    }

    // Open the file.  We have observed this step failing with error code
    // ERROR_NOT_FOUND if the device was recently unplugged.
    if (error == NULL)
    {
        new_gh->file_handle = CreateFile(filename,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

        if (new_gh->file_handle == INVALID_HANDLE_VALUE)
        {
            error = error_create_winapi("Failed to open generic handle.");
        }
    }

    // Initialize WinUSB.
    if (error == NULL)
    {
        BOOL success = WinUsb_Initialize(new_gh->file_handle, &new_gh->winusb_handle);
        if (!success)
        {
            error = error_create_winapi("Failed to initialize WinUSB.");
        }
    }

    if (error == NULL)
    {
        // Success.
        *gh = new_gh;
        new_gh = NULL;
    }

    libusbp_string_free(filename);
    libusbp_generic_handle_close(new_gh);
    return error;
}

void libusbp_generic_handle_close(libusbp_generic_handle * gh)
{
    if (gh != NULL)
    {
        if (gh->winusb_handle != INVALID_HANDLE_VALUE)
        {
            WinUsb_Free(gh->winusb_handle);
        }

        if (gh->file_handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(gh->file_handle);
        }

        free(gh);
    }
}

libusbp_error * libusbp_generic_handle_open_async_in_pipe(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    libusbp_async_in_pipe ** pipe)
{
    return async_in_pipe_create(handle, pipe_id, pipe);
}

libusbp_error * libusbp_generic_handle_set_timeout(
    libusbp_generic_handle * gh,
    uint8_t pipe_id,
    uint32_t timeout)
{
    if (gh == NULL)
    {
        return error_create("Generic handle is null.");
    }

    ULONG winusb_timeout = timeout;
    BOOL success = WinUsb_SetPipePolicy(gh->winusb_handle, pipe_id,
        PIPE_TRANSFER_TIMEOUT, sizeof(winusb_timeout), &winusb_timeout);
    if (!success)
    {
        return error_create_winapi("Failed to set timeout.");
    }
    return NULL;
}

libusbp_error * libusbp_control_transfer(
    libusbp_generic_handle * gh,
    uint8_t bmRequestType,
    uint8_t bRequest,
    uint16_t wValue,
    uint16_t wIndex,
    void * data,
    uint16_t wLength,
    size_t * transferred)
{
    if (transferred != NULL)
    {
        *transferred = 0;
    }

    if (gh == NULL)
    {
        return error_create("Generic handle is null.");
    }

    if (wLength != 0 && data == NULL)
    {
        return error_create("Control transfer buffer was null while wLength was non-zero.");
    }

    WINUSB_SETUP_PACKET packet;
    packet.RequestType = bmRequestType;
    packet.Request = bRequest;
    packet.Value = wValue;
    packet.Index = wIndex;
    packet.Length = wLength;

    ULONG winusb_transferred;
    BOOL success = WinUsb_ControlTransfer(gh->winusb_handle, packet, data,
        wLength, &winusb_transferred, NULL);
    if (!success)
    {
        return error_create_winapi("Control transfer failed.");
    }

    if (transferred)
    {
        *transferred = winusb_transferred;
    }

    return NULL;
}

libusbp_error * libusbp_write_pipe(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    const void * data,
    size_t size,
    size_t * transferred)
{
    if (transferred != NULL)
    {
        *transferred = 0;
    }

    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    libusbp_error * error = NULL;

    if (error == NULL)
    {
        error = check_pipe_id_out(pipe_id);
    }

    if (error == NULL && size > ULONG_MAX)
    {
        error = error_create("Transfer size is too large.");
    }

    if (error == NULL && data == NULL && size)
    {
        error = error_create("Buffer is null.");
    }

    ULONG winusb_transferred = 0;
    if (error == NULL)
    {
        BOOL success = WinUsb_WritePipe(handle->winusb_handle, pipe_id,
            (uint8_t *)data, size, &winusb_transferred, NULL);
        if (!success)
        {
            error = error_create_winapi("");
        }
    }

    if (transferred)
    {
        *transferred = winusb_transferred;
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to write to pipe.");
    }

    return error;
}

libusbp_error * libusbp_read_pipe(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    void * data,
    size_t size,
    size_t * transferred)
{
    if (transferred != NULL)
    {
        *transferred = 0;
    }

    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    libusbp_error * error = NULL;

    if (error == NULL)
    {
        error = check_pipe_id_in(pipe_id);
    }

    if (error == NULL && size == 0)
    {
        error = error_create("Transfer size 0 is not allowed.");
    }

    if (error == NULL && size > ULONG_MAX)
    {
        error = error_create("Transfer size is too large.");
    }

    if (error == NULL && data == NULL)
    {
        error = error_create("Buffer is null.");
    }

    ULONG winusb_transferred = 0;
    if (error == NULL)
    {
        BOOL success = WinUsb_ReadPipe(handle->winusb_handle, pipe_id, data,
            size, &winusb_transferred, NULL);
        if (!success)
        {
            error = error_create_winapi("");
        }
    }

    if (transferred)
    {
        *transferred = winusb_transferred;
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to read from pipe.");
    }

    return error;
}

HANDLE libusbp_generic_handle_get_winusb_handle(libusbp_generic_handle * handle)
{
    if (handle == NULL) { return INVALID_HANDLE_VALUE; }
    return handle->winusb_handle;
}

libusbp_error * generic_handle_events(libusbp_generic_handle * handle)
{
    LIBUSBP_UNUSED(handle);
    return NULL;
}
