/* Functions for working with a USB device node file
 * (e.g. "/dev/bus/usb/001/002"). */

#include <libusbp_internal.h>

libusbp_error * usbfd_check_existence(const char * filename)
{
    libusbp_error * error = NULL;
    int result = access(filename, F_OK);
    if (result != 0)
    {
        if (errno == ENOENT)
        {
            // The file does not exist.  This might just be a temporary
            // condition, so use the LIBUSBP_ERROR_NOT_READY code.
            error = error_create("File does not exist: %s.", filename);
            error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
        }
        else
        {
            // Something actually went wrong when checking if the file
            // exists.
            error = error_create_errno("Failed to check file: %s.", filename);
        }
    }
    return error;
}

libusbp_error * usbfd_open(const char * filename, int * fd)
{
    assert(filename != NULL);
    assert(fd != NULL);

    *fd = open(filename, O_RDWR | O_CLOEXEC);
    if (*fd == -1)
    {
        libusbp_error * error = error_create_errno("Failed to open USB device %s.", filename);
        return error;
    }

    return NULL;
}

// Reads the device descriptor.  This is not thread-safe, because it is possible
// that another thread might change the position of the file descriptor after
// lseek() and before read().
//
// The kernel code that provides the device descriptor can be found in
// usbdev_read() in devio.c.
libusbp_error * usbfd_get_device_descriptor(int fd, struct usb_device_descriptor * desc)
{
    assert(desc != NULL);

    // Seek to the beginning of the file, where the device descriptor lives.
    off_t offset = lseek(fd, 0, SEEK_SET);
    if (offset == -1)
    {
        return error_create_errno("Failed to go to beginning of USB device file.");
    }

    ssize_t expected_size = sizeof(struct usb_device_descriptor);
    ssize_t size = read(fd, desc, expected_size);
    if (size == -1)
    {
        return error_create_errno("Failed to read device descriptor.");
    }
    if (size != expected_size)
    {
        return error_create("Failed to read device descriptor.  "
            "Expected %zd-byte device descriptor, read %zd bytes.",
            expected_size, size);
    }
    return NULL;
}

libusbp_error * usbfd_control_transfer(int fd, libusbp_setup_packet setup,
    uint32_t timeout, void * data, size_t * transferred)
{
    struct usbdevfs_ctrltransfer transfer = {0};
    transfer.bRequestType = setup.bmRequestType;
    transfer.bRequest = setup.bRequest;
    transfer.wValue = setup.wValue;
    transfer.wIndex = setup.wIndex;
    transfer.wLength = setup.wLength;
    transfer.timeout = timeout;
    transfer.data = data;

    if (transferred != NULL)
    {
        *transferred = 0;
    }

    int result = ioctl(fd, USBDEVFS_CONTROL, &transfer);
    if (result < 0)
    {
        return error_create_errno("Control transfer failed.");
    }

    if (transferred != NULL)
    {
        *transferred = result;
    }

    return NULL;
}

/* Performs a bulk or interrupt transfer on the specified endpoint.
 *
 * Despite the name, USBDEVFS_BULK does actually work for interrupt endpoints.
 * The function usbdev_do_ioctl in devio.c calls proc_bulk in devio.c, which
 * calls usb_bulk_msg in message.c, which explicitly detects if it was called on
 * an interrupt endpoint and handle that situation properly. */
libusbp_error * usbfd_bulk_or_interrupt_transfer(int fd, uint8_t pipe,
    uint32_t timeout, void * buffer, size_t size, size_t * transferred)
{
    if (transferred != NULL)
    {
        *transferred = 0;
    }

    // A buffer size of 0 for IN transfers (or at least interrupt IN
    // transfers in a VirtualBox Linux guest running on a Windows
    // host) seems to put the Linux USB drivers in some weird state
    // where every subsequent request times out.
    if (size == 0 && (pipe & 0x80))
    {
        return error_create("Transfer size 0 is not allowed.");
    }

    // A size greater than UINT_MAX will not fit into the struct below.
    if (size > UINT_MAX)
    {
        return error_create("Transfer size is too large.");
    }

    if (buffer == NULL && size)
    {
        return error_create("Buffer is null.");
    }

    struct usbdevfs_bulktransfer transfer = {0};
    transfer.ep = pipe;
    transfer.len = size;
    transfer.timeout = timeout;
    transfer.data = buffer;

    int result = ioctl(fd, USBDEVFS_BULK, &transfer);
    if (result < 0)
    {
        return error_create_errno("");
    }
    if (transferred != NULL)
    {
        *transferred = result;
    }
    return NULL;
}

libusbp_error * usbfd_submit_urb(int fd, struct usbdevfs_urb * urb)
{
    assert(urb != NULL);

    int result = ioctl(fd, USBDEVFS_SUBMITURB, urb);
    if (result < 0)
    {
        return error_create_errno("Submitting USB request block failed.");
    }

    return NULL;
}

/*! Checks to see if there is a finished asynchronous request.  If there is,
 * this function "reaps" the request and retrieves a pointer to its URB.  The
 * kernel writes to the URB and its associated buffer when you call this
 * function, allowing you to get the results of the operation.
 *
 * If nothing is available to be reaped at the moment, the retrieved URB pointer
 * will be NULL.
 *
 * Note: For Linux kernels older than 4.0, this function will return an error
 * if the USB device happens to be disconnected.  In 4.0 and later, you will be
 * able to reap URBs from disconnected devices thanks to commit 3f2cee73b from
 * Alan Stern on 2015-01-29. */
libusbp_error * usbfd_reap_urb(int fd, struct usbdevfs_urb ** urb)
{
    assert(urb != NULL);

    int result = ioctl(fd, USBDEVFS_REAPURBNDELAY, urb);
    if (result < 0)
    {
        *urb = NULL;

        if (errno == EAGAIN)
        {
            // No URBs are available to be reaped right now.
            return NULL;
        }

        return error_create_errno("Failed to reap an asynchronous transfer.");
    }
    return NULL;
}

/*! Cancels an URB that was already submitted. */
libusbp_error * usbfd_discard_urb(int fd, struct usbdevfs_urb * urb)
{
    assert(urb != NULL);

    int result = ioctl(fd, USBDEVFS_DISCARDURB, urb);
    if (result < 0)
    {
        if (errno == EINVAL)
        {
            // This error code happens if the URB was already completed.  This
            // is not an error.
            return NULL;
        }

        return error_create_errno("Failed to cancel asynchronous transfer.");
    }
    return NULL;
}
