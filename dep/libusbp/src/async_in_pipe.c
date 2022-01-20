#include <libusbp_internal.h>

struct libusbp_async_in_pipe
{
    libusbp_generic_handle * handle;
    uint8_t pipe_id;
    async_in_transfer ** transfer_array;
    size_t transfer_size;
    size_t transfer_count;

    bool endless_transfers_enabled;

    // The number of transfers that are pending, meaning that they were
    // submitted (and possibly completed by the kernel) but not finished (handed
    // to the user of the pipe) yet.  This variable allows us to distinguish
    // between the state where no transfers are pending and the state where all
    // of them are pending.  Note that this definition of pending is different
    // than the definition of pending in the async_in_transfer struct.
    size_t pending_count;

    // The index of the transfer that should finish next.  That transfer will be
    // pending (and thus able to be checked) if pending_count > 0.
    size_t next_finish;

    // The index of the transfer that will be submitted next when we need to
    // submit more transfers.  That transfer can be submitted if pending_count <
    // transfer_count.
    size_t next_submit;
};

static inline size_t increment_and_wrap_size(size_t n, size_t bound)
{
    n++;
    return n >= bound ? 0 : n;
}

static void async_in_transfer_array_free(async_in_transfer ** array, size_t transfer_count)
{
    if (array == NULL) { return; }

    for (size_t i = 0; i < transfer_count; i++)
    {
        async_in_transfer_free(array[i]);
    }
    free(array);
}

void libusbp_async_in_pipe_close(libusbp_async_in_pipe * pipe)
{
    if (pipe != NULL)
    {
        async_in_transfer_array_free(pipe->transfer_array, pipe->transfer_count);
        free(pipe);
    }
}

libusbp_error * async_in_pipe_create(libusbp_generic_handle * handle,
    uint8_t pipe_id, libusbp_async_in_pipe ** pipe)
{
    // Check the pipe output pointer.
    if (pipe == NULL)
    {
        return error_create("Pipe output pointer is null.");
    }

    *pipe = NULL;

    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    libusbp_error * error = NULL;

    // Check the pipe_id parameter.
    if (error == NULL)
    {
        error = check_pipe_id(pipe_id);
    }
    if (error == NULL && !(pipe_id & 0x80))
    {
        error = error_create("Asynchronous pipes for OUT endpoints are not supported.");
    }

    // Perform OS-specific setup.
    if (error == NULL)
    {
        error = async_in_pipe_setup(handle, pipe_id);
    }

    libusbp_async_in_pipe * new_pipe = NULL;
    if (error == NULL)
    {
        new_pipe = calloc(1, sizeof(libusbp_async_in_pipe));
        if (new_pipe == NULL)
        {
            error = &error_no_memory;
        }
    }

    if (error == NULL)
    {
        new_pipe->handle = handle;
        new_pipe->pipe_id = pipe_id;
        *pipe = new_pipe;
        new_pipe = NULL;
    }

    free(new_pipe);
    return error;
}

libusbp_error * libusbp_async_in_pipe_allocate_transfers(
    libusbp_async_in_pipe * pipe,
    size_t transfer_count,
    size_t transfer_size)
{
    if (pipe == NULL)
    {
        return error_create("Pipe argument is null.");
    }

    if (pipe->transfer_array != NULL)
    {
        return error_create("Transfers were already allocated for this pipe.");
    }

    if (transfer_count == 0)
    {
        return error_create("Transfer count cannot be zero.");
    }

    if (transfer_size == 0)
    {
        return error_create("Transfer size cannot be zero.");
    }

    libusbp_error * error = NULL;

    async_in_transfer ** new_transfer_array = NULL;
    if (error == NULL)
    {
        new_transfer_array = calloc(transfer_count, sizeof(async_in_transfer *));
        if (new_transfer_array == NULL)
        {
            error = &error_no_memory;
        }
    }

    for(size_t i = 0; error == NULL && i < transfer_count; i++)
    {
        error = async_in_transfer_create(pipe->handle, pipe->pipe_id,
            transfer_size, &new_transfer_array[i]);
    }

    // Put the new array and the information about it into the pipe.
    if (error == NULL)
    {
        pipe->transfer_array = new_transfer_array;
        pipe->transfer_count = transfer_count;
        pipe->transfer_size = transfer_size;
        new_transfer_array = NULL;
    }

    async_in_transfer_array_free(new_transfer_array, transfer_count);

    if (error != NULL)
    {
        error = error_add(error, "Failed to allocate transfers for asynchronous IN pipe.");
    }
    return error;
}

static void async_in_pipe_submit_next_transfer(libusbp_async_in_pipe * pipe)
{
    assert(pipe != NULL);
    assert(pipe->pending_count < pipe->transfer_count);

    // Submit the next transfer.
    async_in_transfer_submit(pipe->transfer_array[pipe->next_submit]);

    // Update the counts and indices.
    pipe->pending_count++;
    pipe->next_submit = increment_and_wrap_size(pipe->next_submit, pipe->transfer_count);
}

libusbp_error * libusbp_async_in_pipe_start_endless_transfers(
    libusbp_async_in_pipe * pipe)
{
    if (pipe == NULL)
    {
        return error_create("Pipe argument is null.");
    }

    if (pipe->transfer_array == NULL)
    {
        return error_create("Pipe transfers have not been allocated yet.");
    }

    pipe->endless_transfers_enabled = true;

    while(pipe->pending_count < pipe->transfer_count)
    {
        async_in_pipe_submit_next_transfer(pipe);
    }

    return NULL;
}

libusbp_error * libusbp_async_in_pipe_handle_events(libusbp_async_in_pipe * pipe)
{
    if (pipe == NULL)
    {
        return error_create("Pipe argument is null.");
    }

    return generic_handle_events(pipe->handle);
}

libusbp_error * libusbp_async_in_pipe_has_pending_transfers(
    libusbp_async_in_pipe * pipe,
    bool * result)
{
    libusbp_error * error = NULL;

    if (error == NULL && result == NULL)
    {
        error = error_create("Boolean output pointer is null.");
    }

    if (error == NULL)
    {
        *result = false;
    }

    if (error == NULL && pipe == NULL)
    {
        error = error_create("Pipe argument is null.");
    }

    if (error == NULL)
    {
        *result = pipe->pending_count ? 1 : 0;
    }

    return error;
}

libusbp_error * libusbp_async_in_pipe_handle_finished_transfer(
    libusbp_async_in_pipe * pipe,
    bool * finished,
    void * buffer,
    size_t * transferred,
    libusbp_error ** transfer_error)
{
    if (finished != NULL)
    {
        *finished = false;
    }

    if (transferred != NULL)
    {
        *transferred = 0;
    }

    if (transfer_error != NULL)
    {
        *transfer_error = NULL;
    }

    if (pipe == NULL)
    {
        return error_create("Pipe argument is null.");
    }

    if (pipe->pending_count == 0)
    {
        // There are no pending transfers that we could check for completion.
        return NULL;
    }

    async_in_transfer * transfer = pipe->transfer_array[pipe->next_finish];

    if (async_in_transfer_pending(transfer))
    {
        // The next transfer we expect to finish is still pending;
        // the kernel has not told us that it is done.
        return NULL;
    }

    libusbp_error * error = async_in_transfer_get_results(transfer, buffer,
        transferred, transfer_error);

    if (error == NULL)
    {
        if (finished != NULL)
        {
            *finished = true;
        }

        pipe->pending_count--;
        pipe->next_finish = increment_and_wrap_size(pipe->next_finish, pipe->transfer_count);
    }

    if (error == NULL && pipe->endless_transfers_enabled)
    {
        async_in_pipe_submit_next_transfer(pipe);
    }

    return error;
}

libusbp_error * libusbp_async_in_pipe_cancel_transfers(libusbp_async_in_pipe * pipe)
{
    if (pipe == NULL)
    {
        return error_create("Pipe argument is null.");
    }

    pipe->endless_transfers_enabled = false;

    libusbp_error * error = NULL;

    #ifdef __linux__
    // In Linux, transfers need to be cancelled individually.
    for (size_t i = 0; error == NULL && i < pipe->transfer_count; i++)
    {
        error = async_in_transfer_cancel(pipe->transfer_array[i]);

        // This doesn't help the performance issue in this function:
        //if (error == NULL) { error = generic_handle_events(pipe->handle); }
    }

    #else

    // On other platforms, any of the transfers has all the information needed
    // to cancel the others.
    error = async_in_transfer_cancel(pipe->transfer_array[0]);

    #endif

    return error;
}
