/* This file has functions for converting Windows error codes libusbp_error
 * objects.  See error_message_test.cpp for documentation/justification of the
 * conversions that are performed. */

#include <libusbp_internal.h>

libusbp_error * error_create_errno(const char * format, ...)
{
    int error_code = errno;

    libusbp_error * error = error_create("Error code %d.", error_code);

    bool skip_standard_message = false;

    switch(error_code)
    {
    case EACCES:
        error = error_add_code(error, LIBUSBP_ERROR_ACCESS_DENIED);
        break;

    case ENOMEM:
        error = error_add_code(error, LIBUSBP_ERROR_MEMORY);
        break;

    case EPIPE:
        skip_standard_message = true;
        error = error_add(error,
            "The request was invalid or there was an I/O problem.");
        error = error_add_code(error, LIBUSBP_ERROR_STALL);
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        break;

    case ENODEV:
    case ESHUTDOWN:
        skip_standard_message = true;
        error = error_add(error, "The device was removed.");
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        break;

    case EPROTO:
    case ETIME:
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        break;

    case ETIMEDOUT:
        skip_standard_message = true;
        error = error_add(error, "The operation timed out.");
        error = error_add_code(error, LIBUSBP_ERROR_TIMEOUT);
        break;

    case EOVERFLOW:
        skip_standard_message = true;
        error = error_add(error, "The transfer overflowed.");
        break;

    case EILSEQ:
        skip_standard_message = true;
        error = error_add(error,
          "Illegal byte sequence: the device may have been disconnected or the request may have been cancelled.");
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        error = error_add_code(error, LIBUSBP_ERROR_CANCELLED);
        break;
    }

    if (!skip_standard_message)
    {
        // We use strerror_r because strerror is not guaranteed to be
        // thread-safe.  Also note that strerror_r does depend on the
        // locale.
        char buffer[256];
        int result = strerror_r(error_code, buffer, sizeof(buffer) - 1);
        if (result == 0)
        {
            error = error_add(error, "%s.", buffer);
        }
    }

    // Finally, add the context message provided by the caller.
    va_list ap;
    va_start(ap, format);
    error = error_add_v(error, format, ap);
    va_end(ap);

    return error;
}

libusbp_error * error_from_urb_status(struct usbdevfs_urb * urb)
{
    libusbp_error * error = NULL;

    int error_code = -urb->status;

    switch(error_code)
    {
    case 0:             // Success
        break;

    case ENOENT:
        error = error_create("Error code %d.", error_code);
        error = error_add(error, "The operation was cancelled.");
        error = error_add_code(error, LIBUSBP_ERROR_CANCELLED);
        break;

    default:
        errno = error_code;
        error = error_create_errno("");
        break;
    }

    return error;
}

libusbp_error * error_create_udev(int error_code, const char * format, ...)
{
    libusbp_error * error = error_create("Error from libudev: %d.", error_code);

    va_list ap;
    va_start(ap, format);
    error = error_add_v(error, format, ap);
    va_end(ap);

    return error;
}
