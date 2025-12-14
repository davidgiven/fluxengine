#include <libusbp_internal.h>

struct async_in_transfer
{
    IOUSBInterfaceInterface182 ** ioh;
    uint8_t pipe_index;
    void * buffer;
    uint32_t size;
    bool pending;
    size_t transferred;
    libusbp_error * error;
};

static void async_in_transfer_callback(void * context, kern_return_t kr, void * arg0)
{
    #ifdef LIBUSBP_LOG
    printf("async_in_transfer_callback (%p): %p, %#x, %p, %s\n",
        async_in_transfer_callback, context, kr, arg0, mach_error_string(kr));
    #endif

    async_in_transfer * transfer = (async_in_transfer *)context;
    assert(transfer != NULL);
    assert(transfer->pending);
    assert(transfer->error == NULL);
    assert(transfer->transferred == 0);

    libusbp_error * error = NULL;

    if (kr != KERN_SUCCESS)
    {
        error = error_create_mach(kr, "Asynchronous IN transfer failed.");
    }

    transfer->transferred = (size_t)arg0;

    transfer->pending = false;
    transfer->error = error;
}

libusbp_error * async_in_transfer_create(
    libusbp_generic_handle * handle, uint8_t pipe_id, size_t transfer_size,
    async_in_transfer ** transfer)
{
    assert(handle != NULL);

    if (transfer_size > UINT32_MAX)
    {
        return error_create("Transfer size is too large.");
    }

    libusbp_error * error = NULL;

    // Allocate memory for the transfer struct.
    async_in_transfer * new_transfer = NULL;
    if (error == NULL)
    {
        new_transfer = calloc(1, sizeof(async_in_transfer));
        if (new_transfer == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Allocate memory for the buffer.
    if (error == NULL)
    {
        new_transfer->size = transfer_size;
        new_transfer->buffer = malloc(transfer_size);
        if (new_transfer->buffer == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Get needed information from the generic handle.
    if (error == NULL)
    {
        new_transfer->ioh = generic_handle_get_ioh(handle);
        new_transfer->pipe_index = generic_handle_get_pipe_index(handle, pipe_id);
    }

    // Pass the transfer to the caller.
    if (error == NULL)
    {
        *transfer = new_transfer;
        new_transfer = NULL;
    }

    async_in_transfer_free(new_transfer);
    return error;
}

libusbp_error * async_in_pipe_setup(libusbp_generic_handle * gh, uint8_t pipe_id)
{
    LIBUSBP_UNUSED(gh);
    LIBUSBP_UNUSED(pipe_id);
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
        // we choose to have a memory leak.
        return;
    }

    libusbp_error_free(transfer->error);
    free(transfer->buffer);
    free(transfer);
}

void async_in_transfer_submit(async_in_transfer * transfer)
{
    assert(transfer != NULL);
    assert(transfer->pending == false);

    libusbp_error_free(transfer->error);
    transfer->error = NULL;
    transfer->transferred = 0;
    transfer->pending = true;

    kern_return_t kr = (*transfer->ioh)->ReadPipeAsync(transfer->ioh,
        transfer->pipe_index, transfer->buffer, transfer->size,
        async_in_transfer_callback, transfer);
    if (kr != KERN_SUCCESS)
    {
        transfer->pending = false;
        transfer->error = error_create_mach(kr, "Failed to submit asynchronous read transfer.");
    }
}

libusbp_error * async_in_transfer_get_results(async_in_transfer * transfer,
    void * buffer, size_t * transferred, libusbp_error ** transfer_error)
{
    assert(transfer != NULL);
    assert(transfer->pending == false);

    size_t tmp_transferred = transfer->transferred;

    // Make sure we don't overflow the user's buffer.
    if (tmp_transferred > transfer->size)
    {
        assert(0);
        tmp_transferred = transfer->size;
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

// This cancels all of the transfers for the whole pipe.  It is not possible to
// cancel an individual transfer on Mac OS X, which is one of the reasons
// individual transfers are not provided as first-class objects by the libusbp
// API.
libusbp_error * async_in_transfer_cancel(async_in_transfer * transfer)
{
    if (transfer == NULL) { return NULL; }

    kern_return_t kr = (*transfer->ioh)->AbortPipe(transfer->ioh, transfer->pipe_index);
    if (kr != KERN_SUCCESS)
    {
        return error_create_mach(kr, "Failed to cancel transfers.");
    }
    return NULL;
}

bool async_in_transfer_pending(async_in_transfer * transfer)
{
    assert(transfer != NULL);
    return transfer->pending;
}
