#include <libusbp_internal.h>

#if defined(_WIN32) || defined(__APPLE__)

libusbp_error * error_create_hr(HRESULT hr, const char * format, ...)
{
    // HRESULT should be an int32_t on the systems we care about (Mac OS X,
    // Win32, Win64), but let's assert it here in case that ever changes.
    assert(sizeof(HRESULT) == 4);
    assert((HRESULT)-1 < (HRESULT)0);

    libusbp_error * error = error_create("HRESULT error code 0x%x.", (int32_t)hr);

    va_list ap;
    va_start(ap, format);
    error = error_add_v(error, format, ap);
    va_end(ap);
    return error;
}

#endif
