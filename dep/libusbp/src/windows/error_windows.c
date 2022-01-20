/* This file has functions for converting Windows error codes libusbp_error
 * objects.  See error_message_test.cpp for documentation/justification of the
 * conversions that are performed. */

#include <libusbp_internal.h>

static libusbp_error * error_create_winapi_v(
    DWORD error_code, const char * format, va_list ap)
{
    libusbp_error * error = error_create("Windows error code 0x%lx.", error_code);

    bool skip_windows_message = false;

    // Convert certain Windows error codes into libusbp error codes.
    switch(error_code)
    {
    case ERROR_ACCESS_DENIED:
        error = error_add(error, "Try closing all other programs that are using the device.");
        error = error_add_code(error, LIBUSBP_ERROR_ACCESS_DENIED);
        break;
    case ERROR_OUTOFMEMORY:
    case ERROR_NOT_ENOUGH_MEMORY:
        error = error_add_code(error, LIBUSBP_ERROR_MEMORY);
        break;
    case ERROR_GEN_FAILURE:
        skip_windows_message = true;
        error = error_add(error, "The request was invalid or there was an I/O problem.");
        error = error_add_code(error, LIBUSBP_ERROR_STALL);
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        break;
    case ERROR_SEM_TIMEOUT:
        skip_windows_message = true;
        error = error_add(error, "The operation timed out.");
        error = error_add_code(error, LIBUSBP_ERROR_TIMEOUT);
        break;
    case ERROR_OPERATION_ABORTED:
        skip_windows_message = true;
        error = error_add(error, "The operation was cancelled.");
        error = error_add_code(error, LIBUSBP_ERROR_CANCELLED);
        break;
    }

    // Get an English sentence from Windows to help describe the error.
    if (!skip_windows_message)
    {
        char buffer[256];
        DWORD size = FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
            NULL, error_code,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), buffer,
            sizeof(buffer), NULL);
        if (size != 0)
        {
            // Remove the leading space and then add the message to the error.
            if (buffer[size - 1] == ' ') { buffer[--size] = 0; }
            error = error_add(error, "%s", buffer);
        }
    }

    // Finally, add the context message provided by the caller.
    return error_add_v(error, format, ap);
}

libusbp_error * error_create_winapi(const char * format, ...)
{
    DWORD error_code = GetLastError();
    va_list ap;
    va_start(ap, format);
    libusbp_error * error = error_create_winapi_v(error_code, format, ap);
    va_end(ap);
    return error;
}

// This is for reporting an error from WinUsb_GetOverlappedResult.  This
// function is necessary because ERROR_FILE_NOT_FOUND has a special meaning if
// it is the result of an asynchronous USB transfer.
libusbp_error * error_create_overlapped(const char * format, ...)
{
    DWORD error_code = GetLastError();
    va_list ap;
    va_start(ap, format);
    libusbp_error * error = NULL;

    switch(error_code)
    {
    case ERROR_FILE_NOT_FOUND:
        error = error_create("Windows error code 0x%lx.", error_code);
        error = error_add(error, "The device was disconnected.");
        error = error_add_code(error, LIBUSBP_ERROR_DEVICE_DISCONNECTED);
        error = error_add_v(error, format, ap);
        break;

    default:
        error = error_create_winapi_v(error_code, format, ap);
        break;
    }

    va_end(ap);
    return error;
}

libusbp_error * error_create_cr(CONFIGRET cr, const char * format, ...)
{
    libusbp_error * error = error_create("CONFIGRET error code 0x%lx.", cr);

    va_list ap;
    va_start(ap, format);
    error = error_add_v(error, format, ap);
    va_end(ap);
    return error;
}
