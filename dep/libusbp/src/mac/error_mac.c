#include <libusbp_internal.h>

libusbp_error * error_create_mach(kern_return_t error_code, const char * format, ...)
{
    // Add the numerical error code.
    libusbp_error * error = error_create("Error code 0x%x.", error_code);

    bool skip_standard_message = false;

    switch(error_code)
    {
    case kIOUSBPipeStalled:
        skip_standard_message = true;
        error = error_add(error, "The request was invalid or there was an I/O problem.");
        error = error_add_code(error, LIBUSBP_ERROR_STALL);
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        break;

    case kIOReturnNoDevice:
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        break;

    case kIOUSBTransactionTimeout:
        skip_standard_message = true;
        error = error_add(error, "The operation timed out.");
        error = error_add_code(error, LIBUSBP_ERROR_TIMEOUT);
        break;

    case kIOReturnExclusiveAccess:
        skip_standard_message = true;
        error = error_add(error,
            "Access is denied.  Try closing all other programs that are using the device.");
        error = error_add_code(error, LIBUSBP_ERROR_ACCESS_DENIED);
        break;

    case kIOReturnOverrun:
        skip_standard_message = true;
        error = error_add(error, "The transfer overflowed.");
        break;

    case kIOReturnAborted:
        skip_standard_message = true;
        error = error_add(error, "The operation was cancelled.");
        error = error_add_code(error, LIBUSBP_ERROR_CANCELLED);
        break;
    }

    if (!skip_standard_message)
    {
        // Add a message from the system.
        error = error_add(error, "%s.", mach_error_string(error_code));
    }

    // Finally, add the context provided by the caller.
    va_list ap;
    va_start(ap, format);
    error = error_add_v(error, format, ap);
    va_end(ap);

    return error;
}
