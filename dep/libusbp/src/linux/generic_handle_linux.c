#include <libusbp_internal.h>

struct libusbp_generic_handle
{
    libusbp_device * device;
    int fd;

    // Timeouts are stored in milliseconds.  0 is forever.
    uint32_t in_timeout[MAX_ENDPOINT_NUMBER + 1];
    uint32_t out_timeout[MAX_ENDPOINT_NUMBER + 1];
};

// Allocates memory structures and opens the device file, but does read or write
// anything.
static libusbp_error * generic_handle_setup(
    const libusbp_generic_interface * gi,
    libusbp_generic_handle ** handle)
{
    assert(gi != NULL);
    assert(handle != NULL);

    libusbp_error * error = NULL;

    // Allocate memory for the handle.
    libusbp_generic_handle * new_handle = NULL;
    if (error == NULL)
    {
        new_handle = calloc(1, sizeof(libusbp_generic_handle));
        if (handle == NULL) { error = &error_no_memory; }
    }

    // Copy the device.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        error = generic_interface_get_device_copy(gi, &new_device);
    }

    // Get the filename.
    char * new_filename = NULL;
    if (error == NULL)
    {
        error = libusbp_generic_interface_get_os_filename(gi, &new_filename);
    }

    // Open the file.
    int new_fd = -1;
    if (error == NULL)
    {
        error = usbfd_open(new_filename, &new_fd);
    }

    // Assemble the handle and pass it to the caller.
    if (error == NULL)
    {
        new_handle->fd = new_fd;
        new_fd = -1;

        new_handle->device = new_device;
        new_device = NULL;

        *handle = new_handle;
        new_handle = NULL;
    }

    if (new_fd != -1) { close(new_fd); }
    libusbp_string_free(new_filename);
    libusbp_device_free(new_device);
    free(new_handle);
    return error;
}

// Reads the device descriptor from the device and makes sure that certain
// fields in it match what we were expecting.  This should help detect the
// situation where devices were changed between the time that the libusbp_device
// object was created and the time that this handle was created.
static libusbp_error * check_device_descriptor(
    libusbp_generic_handle * handle)
{
    assert(handle != NULL);

    libusbp_error * error = NULL;

    struct usb_device_descriptor desc;
    if (error == NULL)
    {
        error = usbfd_get_device_descriptor(handle->fd, &desc);
    }

    uint16_t vendor_id;
    if (error == NULL)
    {
        error = libusbp_device_get_vendor_id(handle->device, &vendor_id);
    }
    if (error == NULL && desc.idVendor != vendor_id)
    {
        error = error_create("Vendor ID mismatch: 0x%04x != 0x%04x.",
            desc.idVendor, vendor_id);
    }

    uint16_t product_id;
    if (error == NULL)
    {
        error = libusbp_device_get_product_id(handle->device, &product_id);
    }
    if (error == NULL && desc.idProduct != product_id)
    {
        error = error_create("Product ID mismatch: 0x%04x != 0x%04x.",
            desc.idProduct, product_id);
    }

    uint16_t revision;
    if (error == NULL)
    {
        error = libusbp_device_get_revision(handle->device, &revision);
    }
    if (error == NULL && desc.bcdDevice != revision)
    {
        error = error_create("Device revision mismatch: 0x%04x != 0x%04x.",
            desc.bcdDevice, revision);
    }

    return error;
}

libusbp_error * libusbp_generic_handle_open(
    const libusbp_generic_interface * gi,
    libusbp_generic_handle ** handle)
{
    if (handle == NULL)
    {
        return error_create("Generic handle output pointer is null.");
    }

    *handle = NULL;

    if (gi == NULL)
    {
        return error_create("Generic interface is null.");
    }

    libusbp_error * error = NULL;

    // Set up the memory structures and open the file handle.
    libusbp_generic_handle * new_handle = NULL;
    if (error == NULL)
    {
        error = generic_handle_setup(gi, &new_handle);
    }

    // Check that the device descriptor is consistent.
    if (error == NULL)
    {
        error = check_device_descriptor(new_handle);
    }

    // Pass the handle to the caller.
    if (error == NULL)
    {
        *handle = new_handle;
        new_handle = NULL;
    }

    libusbp_generic_handle_close(new_handle);
    return error;
}

void libusbp_generic_handle_close(libusbp_generic_handle * handle)
{
    if (handle != NULL)
    {
        close(handle->fd);
        libusbp_device_free(handle->device);
        free(handle);
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
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    uint32_t timeout)
{
    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    libusbp_error * error = NULL;

    if (error == NULL)
    {
        error = check_pipe_id(pipe_id);
    }

    if (error == NULL)
    {
        uint8_t endpoint_number = pipe_id & MAX_ENDPOINT_NUMBER;

        if (pipe_id & 0x80)
        {
            handle->in_timeout[endpoint_number] = timeout;
        }
        else
        {
            handle->out_timeout[endpoint_number] = timeout;
        }
    }

    return error;
}

libusbp_error * libusbp_control_transfer(
    libusbp_generic_handle * handle,
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

    if (handle == NULL)
    {
        return error_create("Generic handle is null.");
    }

    struct libusbp_setup_packet setup;
    setup.bmRequestType = bmRequestType;
    setup.bRequest = bRequest;
    setup.wValue = wValue;
    setup.wIndex = wIndex;
    setup.wLength = wLength;

    return usbfd_control_transfer(handle->fd, setup,
        handle->out_timeout[0], data, transferred);
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

    if (error == NULL)
    {
        uint8_t endpoint_number = pipe_id & MAX_ENDPOINT_NUMBER;
        uint32_t timeout = handle->in_timeout[endpoint_number];
        error = usbfd_bulk_or_interrupt_transfer(
            handle->fd, pipe_id, timeout, data, size, transferred);
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to read from pipe.");
    }
    return error;
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

    if (error == NULL)
    {
        uint8_t endpoint_number = pipe_id & MAX_ENDPOINT_NUMBER;
        uint32_t timeout = handle->out_timeout[endpoint_number];
        error = usbfd_bulk_or_interrupt_transfer(
            handle->fd, pipe_id, timeout, (void *)data, size, transferred);
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to write to pipe.");
    }
    return error;
}

LIBUSBP_WARN_UNUSED
static libusbp_error * handle_completed_urb(struct usbdevfs_urb * urb)
{
    if (urb->usercontext == NULL)
    {
        return error_create("A completed USB request block has a NULL usercontext.");
    }

    if (urb->type == USBDEVFS_URB_TYPE_BULK && (urb->endpoint & 0x80))
    {
        async_in_transfer * transfer = urb->usercontext;
        async_in_transfer_handle_completion(transfer);
        return NULL;
    }
    else
    {
        return error_create("A completed USB request block was unrecognized.");
    }
}

libusbp_error * generic_handle_events(libusbp_generic_handle * handle)
{
    if (handle == NULL)
    {
        return error_create("Generic handle argument is null.");
    }

    while(true)
    {
        struct usbdevfs_urb * urb;
        libusbp_error * error = usbfd_reap_urb(handle->fd, &urb);
        if (error != NULL)
        {
            // There was some problem, like the device being disconnected.
            return error;
        }

        if (urb == NULL)
        {
            // No more URBs left to reap.
            return NULL;
        }

        error = handle_completed_urb(urb);
        if (error != NULL)
        {
            return error;
        }
    }

    return NULL;
}

int libusbp_generic_handle_get_fd(libusbp_generic_handle * handle)
{
    if (handle == NULL)
    {
        return -1;
    }
    return handle->fd;
}
