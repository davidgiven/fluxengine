#include <libusbp_internal.h>

struct async_in_transfer
{
    struct usbdevfs_urb urb;
    bool pending;
    libusbp_error * error;
    int fd;
};

libusbp_error * async_in_pipe_setup(libusbp_generic_handle * handle, uint8_t pipe_id)
{
    LIBUSBP_UNUSED(handle);
    LIBUSBP_UNUSED(pipe_id);
    return NULL;
}

libusbp_error * async_in_transfer_create(
    libusbp_generic_handle * handle, uint8_t pipe_id, size_t transfer_size,
    async_in_transfer ** transfer)
{
    assert(transfer_size != 0);
    assert(transfer != NULL);

    if (transfer_size > INT_MAX)
    {
        // usbdevfs_urb uses ints to represent sizes.
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

    // Assemble the transfer and pass it to the caller.
    if (error == NULL)
    {
        new_transfer->fd = libusbp_generic_handle_get_fd(handle);

        new_transfer->urb.usercontext = new_transfer;
        new_transfer->urb.buffer_length = transfer_size;
        new_transfer->urb.type = USBDEVFS_URB_TYPE_BULK;
        new_transfer->urb.endpoint = pipe_id;

        new_transfer->urb.buffer = new_buffer;
        new_buffer = NULL;

        *transfer = new_transfer;
        new_transfer = NULL;
    }

    free(new_buffer);
    free(new_transfer);
    return error;
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
    free(transfer->urb.buffer);
    free(transfer);
}

void async_in_transfer_submit(async_in_transfer * transfer)
{
    assert(transfer != NULL);
    assert(transfer->pending == false);

    libusbp_error_free(transfer->error);
    transfer->error = NULL;
    transfer->pending = true;

    libusbp_error * error = usbfd_submit_urb(transfer->fd, &transfer->urb);
    if (error != NULL)
    {
        transfer->pending = false;
        transfer->error = error;
    }
}

void async_in_transfer_handle_completion(async_in_transfer * transfer)
{
    assert(transfer != NULL);

    #ifdef LIBUSBP_LOG
    fprintf(stderr, "URB completed: %p, status=%d, actual_length=%d\n",
        transfer, transfer->urb.status, transfer->urb.actual_length);
    #endif

    libusbp_error * error = NULL;

    if (error == NULL)
    {
        error = error_from_urb_status(&transfer->urb);
    }

    if (error == NULL && transfer->urb.error_count != 0)
    {
        error = error_create("Non-zero error count for USB request: %d.",
            transfer->urb.error_count);
    }

    if (error != NULL)
    {
        error = error_add(error, "Asynchronous IN transfer failed.");
    }

    transfer->pending = false;
    transfer->error = error;
}

libusbp_error * async_in_transfer_get_results(async_in_transfer * transfer,
    void * buffer, size_t * transferred, libusbp_error ** transfer_error)
{
    assert(transfer != NULL);
    assert(!transfer->pending);

    size_t tmp_transferred = transfer->urb.actual_length;

    // Make sure we don't overflow the user's buffer.
    if (tmp_transferred > (size_t)transfer->urb.buffer_length)
    {
        tmp_transferred = transfer->urb.buffer_length;
    }

    if (buffer != NULL)
    {
        memcpy(buffer, transfer->urb.buffer, tmp_transferred);
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

libusbp_error * async_in_transfer_cancel(async_in_transfer * transfer)
{
    if (transfer == NULL) { return NULL; }

    return usbfd_discard_urb(transfer->fd, &transfer->urb);
}

bool async_in_transfer_pending(async_in_transfer * transfer)
{
    assert(transfer != NULL);
    return transfer->pending;
}
