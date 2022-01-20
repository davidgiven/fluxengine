/* This file tests the conversion of errors from libusbp's underlying APIs to
 * libusbp_error objects.  It also documents and justifies the explicit mappings
 * that are in the error handling code. */

#include <test_helper.h>

#ifndef NDEBUG

// Here is a list of the messages we want to use consistently across platforms
// to describe certain types of errors:
#define STR_TIMEOUT "The operation timed out."
#define STR_CANCELLED "The operation was cancelled."
#define STR_GENERAL_FAILURE "The request was invalid or there was an I/O problem."
#define STR_REMOVED "The device was removed."
#define STR_OVERFLOW "The transfer overflowed."

#ifdef _WIN32

TEST_CASE("error_create_winapi", "[error_create_winapi]")
{
    libusbp::error error;

    SECTION("includes the error code from GetLastError and its message")
    {
        SetLastError(0x1F4);
        error.pointer_reset(error_create_winapi("Something failed."));
        REQUIRE(error.message() == "Something failed.  "
            "User profile cannot be loaded.  "
            "Windows error code 0x1f4.");
    }

    SECTION("still works if Windows does not have a message")
    {
        SetLastError(0x1F431892);
        error.pointer_reset(error_create_winapi("Something failed."));
        REQUIRE(error.message() == "Something failed.  "
            "Windows error code 0x1f431892.");
    }

    SECTION("access denied errors")
    {
        // ERROR_ACCESS_DENIED can happen when running CreateFile to open a
        // handle to a WinUSB device node that is already being used by another
        // application.  This is one of the few cases where a libusbp error
        // message actually contains a troubleshooting step for the user to try.
        // It is included in the library because it is a universally useful piece
        // of advice for any WinUSB device.
        //
        // This feature is part of error_create_winapi, which means we are
        // assuming that we will only ERROR_ACCESS_DENIED as a result of trying
        // to open a device that is being used.  If we expand the library to do
        // other things that might have their access denied, this feature should
        // maybe move to a different function.
        SetLastError(ERROR_ACCESS_DENIED);
        error.pointer_reset(error_create_winapi("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_ACCESS_DENIED));
        CHECK(error.message() ==
            "Hi.  "
            "Access is denied.  "
            "Try closing all other programs that are using the device.  "
            "Windows error code 0x5.");
    }

    SECTION("out of memory errors")
    {
        // We haven't specifically seen these errors happen before, but it seems
        // like either of them might be used to indicate that the system is out
        // memory.
        SetLastError(ERROR_OUTOFMEMORY);
        error.pointer_reset(error_create_winapi("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_MEMORY));

        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        error.pointer_reset(error_create_winapi("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_MEMORY));
    }

    SECTION("general failure")
    {
        // We have seen ERROR_GEN_FAILURE occur in multiple different
        // situations.
        //
        // It could mean that the device was disconnected from the computer
        // in the middle of or during a synchronous USB transfer.
        //
        // It could mean that the host just sent a control transfer
        // request that the device does not support, and the device has returned
        // a STALL packet, which is a perfectly valid way for the device to
        // behave.
        //
        // It probably also applies to STALL packets during IN and OUT
        // transfers.
        //
        // The default message is "A device attached to the system is not
        // functioning."  This places too much blame on the USB device, so we
        // definitely want libusbp to use STR_GENERAL_FAILURE instead.
        SetLastError(ERROR_GEN_FAILURE);
        error.pointer_reset(error_create_winapi("Hey."));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));
        CHECK(error.has_code(LIBUSBP_ERROR_STALL));
        CHECK(error.message() == "Hey.  "
            STR_GENERAL_FAILURE
            "  Windows error code 0x1f.");
    }

    SECTION("timeout error")
    {
        // The have seen WinUSB return ERROR_SEM_TIMEOUT for both synchronous
        // and asynchronous operations that time out.
        SetLastError(ERROR_SEM_TIMEOUT);
        error.pointer_reset(error_create_winapi("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_TIMEOUT));
        CHECK(error.message() ==  "Hi.  " STR_TIMEOUT "  Windows error code 0x79.");
    }

    SECTION("cancelled error")
    {
        // GetOverlappedResult returns this if the asynchronous operation was
        // cancelled.
        SetLastError(ERROR_OPERATION_ABORTED);
        error.pointer_reset(error_create_winapi("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_CANCELLED));
        CHECK(error.message() == "Hi.  " STR_CANCELLED "  Windows error code 0x3e3.");
    }
}

TEST_CASE("error_create_overlapped")
{
    libusbp::error error;

    SECTION("usually just calls error_create_winapi")
    {
        SetLastError(ERROR_SEM_TIMEOUT);
        error.pointer_reset(error_create_overlapped("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_TIMEOUT));
    }

    SECTION("device disconnect error")
    {
        // WinUsb_GetOverlappedResult (and presumably also GetOverlappedResult)
        // returns this error code when you are checking the status of an
        // asynchronous operation for a device that has been disconnected from
        // the system.  This is inconsistent with the synchronous operations,
        // which return ERROR_GEN_FAILURE, but it makes sense because
        // GetOverlappedResult is a more general thing that applies to any type
        // of file that can have asynchronous operations performed on it.
        //
        // It might be OK to put this feature into error_create_winapi, but in
        // general, ERROR_FILE_NOT_FOUND does not mean a device was
        // disconnected, so we have put this behavior in a special function
        // named error_create_overlapped (instead of error_create_winapi).
        SetLastError(ERROR_FILE_NOT_FOUND);
        error.pointer_reset(error_create_overlapped("Hey."));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));
        CHECK(error.message() == "Hey.  "
            "The device was disconnected.  "
            "Windows error code 0x2.");
    }
}

TEST_CASE("error_create_cr", "[error_create_cr]")
{
    SECTION("returns the right message")
    {
        libusbp::error error(error_create_cr(CR_NO_SUCH_DEVNODE, "Hi."));
        REQUIRE(error.message() == "Hi.  CONFIGRET error code 0xd.");
    }
}

#endif


#ifdef __linux__

TEST_CASE("error_create_errno", "[error_create_errno]")
{
    libusbp::error error;

    SECTION("returns the right message")
    {
        errno = ENOENT;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.message() ==
            "Hi.  No such file or directory.  Error code 2.");
    }

    SECTION("still works when no message is available")
    {
        errno = -122344;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.message() == "Hi.  Error code -122344.");
    }

    SECTION("access denied error")
    {
        // EACCES is the error we see when calling open() on a USB device file
        // that we don't have read-write permissions for.
        errno = EACCES;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_ACCESS_DENIED));

        // One reason we might not have permissions is that the udev rules might
        // not have been applied yet, so usbfd_open adds the
        // LIBUSBP_ERROR_NOT_READY code to EACCES errors.  (That is NOT tested
        // here though.)
        CHECK_FALSE(error.has_code(LIBUSBP_ERROR_NOT_READY));
    }

    SECTION("EPERM error")
    {
        // EPERM is described in errno-base.h as "operation not permitted".  The
        // file error-codes.txt says "submission failed because urb->reject was
        // set".  libusb does not do anything special with this error, and I am
        // not sure when we would receive it, so for now let's NOT map it to the
        // LIBUSBP_ERROR_ACCESS_DENIED code.
        errno = EPERM;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK_FALSE(error.has_code(LIBUSBP_ERROR_ACCESS_DENIED));
    }

    SECTION("memory errors")
    {
        // This is described by error-codes.txt as "no memory for allocation of
        // internal structures", and it will probably be used by other non-USB
        // system calls as well.
        errno = ENOMEM;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_MEMORY));
    }

    SECTION("general failure")
    {
        // error-codes.txt says that EPIPE (broken pipe) can either mean
        // "endpoint stalled", a device disconnect, or the pipe type specified
        // in the URB doesn't match the endpoint's actual type.
        //
        // It's interesting that both Windows and Linux return just a single
        // error code which could either mean a STALL packet or a device
        // disconnect.  Maybe it's hard to tell the difference between those two
        // cases because of the design of the host controller interface.
        errno = EPIPE;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_STALL));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));
        CHECK(error.message() == "Hi.  " STR_GENERAL_FAILURE "  Error code 32.");
    }

    SECTION("device disconnects")
    {
        // error-codes.txt documents ENODEV (no such device) to mean "specified
        // USB-device or bus doesn't exist" or "device was removed".  In
        // devio.c, we can see that most ioctls will return ENODEV if the device
        // is not connected.  Also read() will return ENODEV if the device is
        // not connected, which might be a handy way to check if the device is
        // connected.
        errno = ENODEV;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));
        CHECK(error.message() == "Hi.  " STR_REMOVED "  Error code 19.");

        // error-codes.txt says ESHUTDOWN (cannot send after transport endpoint
        // shutdown) means "The device or host controller has been disabled due
        // to some problem that could not be worked around, such as a physical
        // disconnect."  libusb has four places where it handles ESHUTDOWN, and
        // all four of them say that the device was removed.  ESHUTDOWN is the
        // error we typically see from a synchronous operation when the device
        // is removed in the middle of the operation.
        errno = ESHUTDOWN;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));
        CHECK(error.message() == "Hi.  " STR_REMOVED "  Error code 108.");

        // error-codes.txt says EPROTO (protocol error) and ETIME (timer
        // expired) (and EPIPE and EILSEQ) are codes that different kinds of
        // host controller use to indicate a transfer has failed because of
        // device disconnect.

        errno = EPROTO;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));

        errno = ETIME;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));
    }

    SECTION("timeout errors")
    {
        // error-codes.txt says that ETIMEDOUT indicates a timeout in a
        // synchronous USB message.
        errno = ETIMEDOUT;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_TIMEOUT));
        CHECK(error.message() == "Hi.  " STR_TIMEOUT  "  Error code 110.");
    }

    SECTION("overflow errors")
    {
        // error-codes.txt says that EOVERFLOW (Value too large for defined data
        // type) means the amount of data reutnred by the endpoint was greater
        // than either the max packet size or the remaining buffer size.  We
        // don't have a libusbp error code for that, but we want to fix the
        // misleading message from Linux.
        errno = EOVERFLOW;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.message() == "Hi.  " STR_OVERFLOW "  Error code 75.");
    }

    SECTION("EILSEQ")
    {
        // error-codes.txt says that EILSEQ is one of "several codes that
        // different kinds of host controller use to indicate a transfer has
        // failed because of device disconnect".
        //
        // We have also seen EILSEQ happen sometimes in VirtualBox when
        // cancelling an URB, or when removing a device.
        //
        // The default message for EILSEQ is "Invalid or incomplete multibyte or
        // wide character." which is atrocious because it makes me think there
        // is some bug in the software, instead of this being a USB hardware
        // thing.
        errno = EILSEQ;
        error.pointer_reset(error_create_errno("Hi."));
        CHECK(error.has_code(LIBUSBP_ERROR_CANCELLED));
        CHECK(error.has_code(LIBUSBP_ERROR_DEVICE_DISCONNECTED));
        CHECK(error.message() == "Hi.  Illegal byte sequence: "
            "the device may have been disconnected or "
            "the request may have been cancelled.  "
            "Error code 84.");
    }
}

TEST_CASE("error_from_urb_status")
{
    struct usbdevfs_urb urb;
    memset(&urb, 0, sizeof(urb));
    libusbp::error error;

    SECTION("gives a good message for cancellation (ENOENT)")
    {
        // error-codes.txt says the ENOENT in an URB status means that the URB
        // was synchronously unlinked with usb_unlink_urb.  This is the error we
        // typically see for a cancelled asynchronous request (which actually
        // uses usb_kill_urb, which is similar but probably slower).
        urb.status = -ENOENT;
        error.pointer_reset(error_from_urb_status(&urb));
        REQUIRE(error.message() == STR_CANCELLED "  Error code 2.");
    }

    SECTION("does report an error for EREMOTEIO")
    {
        // error-codes.txt says that EREMOTEIO means that the data read from the
        // endpoint did not fill the specified buffer, and URB_SHORT_NOT_OK was
        // set in urb->transfer_flags.  We don't indend to set that flag, and if
        // we did set that flag, then we should probably interpret a short
        // transfer as an error.
        //
        // libusb does NOT treat EREMOTEIO as an error but that is probably
        // because libusb could actually submit multiple URBs per transfer and
        // some of them might not be used.
        urb.status = -EREMOTEIO;
        error.pointer_reset(error_from_urb_status(&urb));
        REQUIRE(error.message() == "Remote I/O error.  Error code 121.");
    }
}

TEST_CASE("error_create_udev", "[error_create_errno]")
{
    SECTION("returns the right message")
    {
        libusbp::error error(error_create_udev(123, "Hi."));
        REQUIRE(error.message() == "Hi.  Error from libudev: 123.");
    }
}
#endif

#ifdef __APPLE__

TEST_CASE("error_create_mach")
{
    SECTION("returns the right message")
    {
        libusbp::error error(error_create_mach(1, "Hi."));
        REQUIRE(error.message() == "Hi.  (os/kern) invalid address.  Error code 0x1.");
    }

    SECTION("stall error")
    {
        // We encounter this error if a control transfer ends with a STALL
        // packet.  This is tested for in control_sync_test.cpp.
        kern_return_t kr = kIOUSBPipeStalled;
        libusbp::error error(error_create_mach(kr, "Hey."));
        REQUIRE(error.message() == "Hey.  " STR_GENERAL_FAILURE  "  Error code 0xe000404f.");
        REQUIRE(error.has_code(LIBUSBP_ERROR_STALL));
    }

    SECTION("timeout")
    {
        // We encounter this error if a control transfer times out.
        // This is tested for in control_sync_test.cpp.
        kern_return_t kr = kIOUSBTransactionTimeout;
        libusbp::error error(error_create_mach(kr, "Hey."));
        REQUIRE(error.message() == "Hey.  " STR_TIMEOUT  "  Error code 0xe0004051.");
        REQUIRE(error.has_code(LIBUSBP_ERROR_TIMEOUT));
    }

    SECTION("access denied")
    {
        // We encounter this error if an IOUSBInterface is already open for
        // exclusive access when we try to open it for exclusive access.
        // This is tested for in generic_handle_test.cpp.
        kern_return_t kr = kIOReturnExclusiveAccess;
        libusbp::error error(error_create_mach(kr, "Hey."));
        REQUIRE(error.message() == "Hey.  Access is denied.  "
            "Try closing all other programs that are using the device.  "
            "Error code 0xe00002c5.");
        REQUIRE(error.has_code(LIBUSBP_ERROR_ACCESS_DENIED));
    }

    SECTION("overflow")
    {
        // This is an error we see when we read from a pipe using an
        // IOUSBInterface and the device returns more data than can fit in our
        // buffer.
        kern_return_t kr = kIOReturnOverrun;
        libusbp::error error(error_create_mach(kr, "Hi."));
        REQUIRE(error.message() == "Hi.  " STR_OVERFLOW "  Error code 0xe00002e8.");
    }

    SECTION("cancellation")
    {
        // This is an error we see when we call ReadPipeAsync and
        // then cancel it with AbortPipe().
        kern_return_t kr = kIOReturnAborted;
        libusbp::error error(error_create_mach(kr, "Hi."));
        REQUIRE(error.message() == "Hi.  " STR_CANCELLED "  Error code 0xe00002eb.");
        REQUIRE(error.has_code(LIBUSBP_ERROR_CANCELLED));
    }
}

#endif

#if defined(_WIN32) || defined(__APPLE__)

TEST_CASE("error_create_hr", "[error_create_hr]")
{
    SECTION("returns the right message")
    {
        libusbp::error error(error_create_hr(0x80070057, "Hi."));
        REQUIRE(error.message() == "Hi.  HRESULT error code 0x80070057.");
    }
}

#endif

#endif  // !NDEBUG
