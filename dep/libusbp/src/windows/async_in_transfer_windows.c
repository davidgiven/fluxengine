#include <libusbp_internal.h>

struct async_in_transfer
{
    HANDLE winusb_handle;
    uint8_t pipe_id;
    void * buffer;
    size_t buffer_size;
    OVERLAPPED overlapped;
    ULONG transferred;
    bool pending;
    libusbp_error * error;
};

libusbp_error * async_in_pipe_setup(libusbp_generic_handle * handle, uint8_t pipe_id)
{
    assert(handle != NULL);

    HANDLE winusb_handle = libusbp_generic_handle_get_winusb_handle(handle);

    // Turn on raw I/O, because without it the transfers will not be efficient
    // enough in Windows Vista and Windows 7, as tested by test_async_in.
    UCHAR raw_io = 1;
    BOOL success = WinUsb_SetPipePolicy(winusb_handle, pipe_id, RAW_IO,
        sizeof(raw_io), &raw_io);
    if (!success)
    {
        return error_create_winapi("Failed to enable raw I/O for pipe 0x%x.", pipe_id);
    }
    return NULL;
}

void async_in_transfer_free(async_in_transfer * transfer)
{
    if (transfer == NULL) { return; }

    if (transfer->pending)
    {
        // Unfortunately, this transfer is still pending, so we cannot free it;
        // the kernel needs to be able to write to this transfer's memory when
        // it completes.  We could just abort the process here, but instead we
        // we choose to have a memory leak, since it can be easily detected and
        // might be small enough to be harmless.
        return;
    }

    libusbp_error_free(transfer->error);
    CloseHandle(transfer->overlapped.hEvent);
    free(transfer->buffer);
    free(transfer);
}

void async_in_transfer_submit(async_in_transfer * transfer)
{
    assert(transfer != NULL);
    assert(transfer->pending == false);

    libusbp_error_free(transfer->error);
    transfer->error = NULL;
    transfer->pending = true;
    transfer->transferred = 0;

    BOOL success = WinUsb_ReadPipe(
        transfer->winusb_handle,
        transfer->pipe_id,
        transfer->buffer,
        transfer->buffer_size,
        &transfer->transferred,
        &transfer->overlapped);
    if (success)
    {
        // The transfer completed immediately.
        transfer->pending = false;
    }
    else if (GetLastError() == ERROR_IO_PENDING)
    {
        // The transfer is pending.
    }
    else
    {
        // An error happened.
        transfer->error = error_create_winapi("Failed to submit asynchronous in transfer.");
        transfer->pending = false;
    }
}

libusbp_error * async_in_transfer_create(
    libusbp_generic_handle * handle, uint8_t pipe_id, size_t transfer_size,
    async_in_transfer ** transfer)
{
    assert(transfer_size != 0);
    assert(transfer != NULL);

    if (transfer_size > ULONG_MAX)
    {
        // WinUSB uses ULONGs to represent sizes.
        return error_create("Transfer size is too large.");
    }

    libusbp_error * error = NULL;

    // Allocate the transfer struct.
    async_in_transfer * new_transfer = NULL;
    if (error == NULL)
    {
        new_transfer = calloc(1, sizeof(async_in_transfer));
        if (new_transfer == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Allocate the buffer for the transfer.
    void * new_buffer = NULL;
    if (error == NULL)
    {
        new_buffer = malloc(transfer_size);
        if (new_buffer == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Create an event.
    HANDLE new_event = INVALID_HANDLE_VALUE;
    if (error == NULL)
    {
        new_event = CreateEvent(NULL, false, false, NULL);
        if (new_event == NULL)
        {
            error = error_create_winapi(
                "Failed to create event for asynchronous in transfer.");
        }
    }

    // Assemble the transfer and pass it to the caller.
    if (error == NULL)
    {
        new_transfer->winusb_handle = libusbp_generic_handle_get_winusb_handle(handle);
        new_transfer->buffer_size = transfer_size;
        new_transfer->pipe_id = pipe_id;

        new_transfer->buffer = new_buffer;
        new_buffer = NULL;

        new_transfer->overlapped.hEvent = new_event;
        new_event = INVALID_HANDLE_VALUE;

        *transfer = new_transfer;
        new_transfer = NULL;
    }

    if (new_event != INVALID_HANDLE_VALUE) { CloseHandle(new_event); }
    free(new_buffer);
    free(new_transfer);
    return error;
}

libusbp_error * async_in_transfer_get_results(async_in_transfer * transfer,
    void * buffer, size_t * transferred, libusbp_error ** transfer_error)
{
    assert(transfer != NULL);
    assert(!transfer->pending);

    size_t tmp_transferred = transfer->transferred;

    // Make sure we don't overflow the user's buffer.
    if (tmp_transferred > transfer->buffer_size)
    {
        tmp_transferred = transfer->buffer_size;
    }

    if (buffer != NULL)
    {
        memcpy(buffer, transfer->buffer, tmp_transferred);
    }

    if (transferred != NULL)
    {
        *transferred = tmp_transferred;
    }

    if (transfer_error != NULL)
    {
        *transfer_error = libusbp_error_copy(transfer->error);
    }

    return NULL;
}

// Cancels all of the transfers on this pipe, given one of the transfers.
libusbp_error * async_in_transfer_cancel(async_in_transfer * transfer)
{
    if (transfer == NULL)
    {
        return error_create("Transfer to cancel is null.");
    }

    BOOL success = WinUsb_AbortPipe(transfer->winusb_handle, transfer->pipe_id);
    if (!success)
    {
        return error_create_winapi("Failed to cancel transfers on pipe 0x%x.",
            transfer->pipe_id);
    }

    return NULL;
}

bool async_in_transfer_pending(async_in_transfer * transfer)
{
    assert(transfer != NULL);

    if (!transfer->pending)
    {
        return false;
    }

    DWORD transferred = 0;
    BOOL success = WinUsb_GetOverlappedResult(
        transfer->winusb_handle, &transfer->overlapped, &transferred, false);

    transfer->transferred = transferred;

    if (success)
    {
        transfer->pending = false;
    }
    else if (GetLastError() == ERROR_IO_INCOMPLETE)
    {
        // The transfer is still pending.
    }
    else
    {
        transfer->error = error_create_overlapped("Asynchronous IN transfer failed.");
        transfer->pending = false;
    }

    return transfer->pending;
}
