// Copyright (C) Pololu Corporation.  See www.pololu.com for details.

/*! \file libusbp.h
 *
 * This header file provides the C API for libusbp.
 */

#pragma once

/*! The major component of libusbp's version number.  In accordance with
 * semantic versioning, this gets incremented every time there is a breaking
 * change. */
#define LIBUSBP_VERSION_MAJOR 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef _WIN32
#include <wtypesbase.h>
#endif

#ifdef _WIN32
#  define LIBUSBP_DLL_EXPORT __declspec(dllexport)
#  define LIBUSBP_DLL_IMPORT __declspec(dllimport)
#else
#  define LIBUSBP_DLL_IMPORT __attribute__((visibility ("default")))
#  define LIBUSBP_DLL_EXPORT __attribute__((visibility ("default")))
#endif

#ifdef _MSC_VER
#define LIBUSBP_WARN_UNUSED _Check_return_
#else
#define LIBUSBP_WARN_UNUSED __attribute__((warn_unused_result))
#endif

#ifdef LIBUSBP_STATIC
#  define LIBUSBP_API
#else
#  ifdef LIBUSBP_EXPORTS
#    define LIBUSBP_API LIBUSBP_DLL_EXPORT
#  else
#    define LIBUSBP_API LIBUSBP_DLL_IMPORT
#  endif
#endif

/*! Some functions in this library return strings to the caller via a char **
 * argument.  If the function call was successful, it is the caller's
 * responsibility to free those strings by passing them to this function.
 * Passing the NULL pointer to this function is OK.  Do not pass any strings to
 * this function unless they were previously returned by a call to this
 * library.  Do not free the same non-NULL string twice. */
LIBUSBP_API
void libusbp_string_free(char *);


/** libusbp_error **************************************************************/

/*! A libusbp_error object represents an error that occurred in the library.
 * Many functions return a libusbp_error pointer as a return value.  The
 * convention is that a NULL pointer indicates success.  If the pointer is not
 * NULL, the caller needs to free it at some point by calling
 * libusbp_error_free().
 *
 * NULL is a valid value for a libusbp_error pointer, and can be passed to any
 * function in this library that takes a libusbp_error pointer. */
typedef struct libusbp_error
               libusbp_error;

/*! Each ::libusbp_error can have 0 or more error codes that give additional
 * information about the error that might help the caller take the right action
 * when the error occurs.  This enum defines which error codes are possible. */
enum libusbp_error_code
{
    /*! There were problems allocating memory.  A memory shortage might be the
     * root cause of the error, or there might be another error that is masked
     * by the memory problems. */
    LIBUSBP_ERROR_MEMORY = 1,

    /*! It is possible that the error was caused by a temporary condition, such
     * as the operating system taking some time to initialize drivers.  Any
     * function that could return this error will say so explicitly in its
     * documentation so you do not have to worry about handling it in too many
     * places. */
    LIBUSBP_ERROR_NOT_READY = 2,

    /*! Access was denied.  A common cause of this error on Windows is that
     *  another application has a handle open to the same device. */
    LIBUSBP_ERROR_ACCESS_DENIED = 3,

    /*! The device does not have a serial number. */
    LIBUSBP_ERROR_NO_SERIAL_NUMBER = 4,

    /*! The device took too long to respond to a request or transfer data. */
    LIBUSBP_ERROR_TIMEOUT = 5,

    /*! The error might have been caused by the device being disconnected, but
     * it is possible it was caused by something else. */
    LIBUSBP_ERROR_DEVICE_DISCONNECTED = 6,

    /*! The error might have been caused by the host receiving a STALL packet
     * from the device, but it is possible it was caused by something else. */
    LIBUSBP_ERROR_STALL = 7,

    /*! The error might have been caused by the transfer getting cancelled from
     * the host side.  Some data might have been transferred anyway. */
    LIBUSBP_ERROR_CANCELLED = 8,
};

/*! Attempts to copy an error.  If you copy a NULL ::libusbp_error
 * pointer, the result will also be NULL.  If you copy a non-NULL ::libusbp_error
 * pointer, the result will be non-NULL, but if there are issues allocating
 * memory, then the copied error might have different properties than the
 * original error, and it will have the ::LIBUSBP_ERROR_MEMORY code.
 *
 * It is the caller's responsibility to free the copied error. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_error_copy(const libusbp_error *);

/*! Frees a returned error object.  Passing the NULL pointer to this function is
 * OK.  Do not free the same non-NULL error twice. */
LIBUSBP_API
void libusbp_error_free(libusbp_error *);

/*! Returns true if the error has specified error code.  The error codes are
 *  listed in the ::libusbp_error_code enum. */
LIBUSBP_API bool libusbp_error_has_code(const libusbp_error *, uint32_t code);

/*! Returns an English-language ASCII-encoded string describing the error.  The
 * message consists of one or more sentences.  If there are multiple sentences,
 * the earlier ones will typically explain the context that the error happened
 * in.
 *
 * The returned pointer will be valid until the error is freed, at which point
 * it might become invalid.  Do not pass the returned pointer to
 * libusbp_string_free(). */
LIBUSBP_API const char * libusbp_error_get_message(const libusbp_error *);


/** libusbp_async_in_pipe ******************************************************/

/*! A libusbp_async_in_pipe is an object that holds the memory and other data
 *  structures for a set of asynchronous USB requests to read data from a
 *  non-zero endpoint.  It can be used to read data from a bulk or IN endpoint
 *  with high throughput. */
typedef struct libusbp_async_in_pipe
               libusbp_async_in_pipe;

/*! Closes the pipe immediately.  Note that if the pipe has any
 * pending transfers, then it is possible that they cannot be freed
 * by this function.  Freeing a pipe with pending transfers could
 * cause a memory leak, but is otherwise safe. */
LIBUSBP_API
void libusbp_async_in_pipe_close(libusbp_async_in_pipe *);

/*! Allocates buffers and other data structures for performing multiple
 * concurrent transfers on the pipe.
 *
 * The @a transfer_count parameter specifies how many transfers to allocate.  You
 * can also think of this as the maximum number of concurrent transfers that can
 * be active at the same time.
 *
 * The @a transfer_size parameter specifies how large each transfer's buffer should
 * be, and is also the number of bytes that will be requested from the operating
 * system when the transfer is submitted.
 *
 * It is best to set the transfer size to a multiple of the maximum packet size
 * of the endpoint.  Otherwise, you might get an error when the device sends
 * more data than can fit in the transfer's buffer.  This type of error is
 * called an overflow.
 *
 * If you want to be reading the pipe every millisecond without gaps, you should
 * set the transfer count high enough so that it would take about 100 ms to 250
 * ms to finish all the transfers.  As long as the operating system runs your
 * process that often, you should be able to keep the USB host controller busy
 * permanently.  (Though we have observed gaps in the transfers when trying to
 * do this inside a VirtualBox machine.)
 *
 * You should not set the transfer count too high, or else it might end up
 * taking a long time to cancel the transfers when you are closing the pipe. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_async_in_pipe_allocate_transfers(
    libusbp_async_in_pipe *,
    size_t transfer_count,
    size_t transfer_size);

/*! Starts reading data from the pipe. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_async_in_pipe_start_endless_transfers(
    libusbp_async_in_pipe *);

/*! Checks for new events, such as a transfer completing.  This
 * function and libusbp_async_in_pipe_handle_finished_transfer() should
 * be called regularly in order to get data from the pipe. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_async_in_pipe_handle_events(libusbp_async_in_pipe *);

/*! Retrieves a boolean saying whether there are any pending
 * transfers.  A pending transfer is a transfer that was submitted to
 * the operating system, and it may have been completed, but it has
 * not been passed to the caller yet via
 * libusbp_async_in_pipe_handle_finished_transfer(). */
LIBUSBP_API LIBUSBP_WARN_UNUSED libusbp_error *
libusbp_async_in_pipe_has_pending_transfers(
    libusbp_async_in_pipe *,
    bool * result);

/*! Checks to see if there is a finished transfer that can be handled.
 * If there is one, then this function retrieves the data from the
 * transfer, the number of bytes transferred, and any error that might
 * have occurred related to the transfer.
 *
 * @param finished An optional output pointer used to return a pointer that
 * indicates whether a transfer was finished.  If the returned value is false,
 * then no transfer was finished, and there is no transfer_error or data to
 * handle.
 *
 * @param buffer An optional output pointer used to return the data
 * from the transfer.  The buffer must be at least as large as the
 * transfer size specifed when
 * libusbp_async_in_pipe_allocate_transfers was called.
 *
 * @param transferred An optional output pointer used to return the
 * number of bytes transferred.
 *
 * @param transfer_error An optional pointer used to return an error
 * related to the transfer, such as a timeout or a cancellation.  If
 * this pointer is provided, and a non-NULL error is returned via it,
 * then the error must later be freed with libusbp_error_free().  There
 * will never be a non-NULL transfer error if there is a regular error
 * returned as the return value of this function. */
LIBUSBP_API LIBUSBP_WARN_UNUSED libusbp_error *
libusbp_async_in_pipe_handle_finished_transfer(
    libusbp_async_in_pipe *,
    bool * finished,
    void * buffer,
    size_t * transferred,
    libusbp_error ** transfer_error);

/*! Cancels all the transfers for this pipe.  The cancellation is
 * asynchronous, so it won't have an immediate effect.  If you want
 * to actually make sure that all the transfers get cancelled, you
 * will need to call libusbp_async_in_pipe_handle_events() and
 * libusbp_async_in_pipe_handle_finished_transfer() repeatedly until
 * libusbp_async_in_pipe_has_pending_transfers() indicates there are
 * no pending transfers left. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_async_in_pipe_cancel_transfers(libusbp_async_in_pipe *);


/** libusbp_device *************************************************************/

/*! Represents a single USB device.  A composite device with multiple functions
 *  is represented by a single libusbp_device object.
 *
 * A NULL libusbp_device pointer is valid and can be passed to any function in
 * this library that takes such pointers.  */
typedef struct libusbp_device
               libusbp_device;

/*! Finds all the USB devices connected to the computer and returns a list of them.
 *
 * The optional @a device_count parameter is used to return the number of
 * devices in the list.  The list is actually one element larger because it ends
 * with a NULL pointer.
 *
 * If this function is successful (the returned error pointer is NULL), then you
 * must later free each device by calling libusbp_device_free() and free the
 * list by calling libusbp_list_free().  The order in which the retrieved
 * objects are freed does not matter. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_list_connected_devices(
    libusbp_device *** device_list,
    size_t * device_count);

/*! Frees a device list returned by libusbp_list_connected_device(). */
LIBUSBP_API
void libusbp_list_free(libusbp_device ** list);

/*! Finds a device with the specified vendor ID and product ID and returns a
 * pointer to it.  If no device can be found, returns a NULL pointer.  If the
 * retrieved device pointer is not NULL, you must free it later by calling
 * libusbp_device_free().  The retrieved device pointer will always be NULL if
 * an error is returned. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_find_device_with_vid_pid(
    uint16_t vendor_id,
    uint16_t product_id,
    libusbp_device ** device);

/*! Makes a copy of a device object.  If this function is successful, you will
 * need to free the copy by calling libusbp_device_free() at some point. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_device_copy(
    const libusbp_device * source,
    libusbp_device ** dest);

/*! Frees a device object.  Passing a NULL pointer to this function is OK.  Do
 * not free the same non-NULL device twice. */
LIBUSBP_API void libusbp_device_free(libusbp_device *);

/*! Gets the USB vendor ID of the device (idVendor). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_device_get_vendor_id(
    const libusbp_device *,
    uint16_t * vendor_id);

/*! Gets the USB product ID of the device (idProduct). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_device_get_product_id(
    const libusbp_device *,
    uint16_t * product_id);

/*! Gets the USB revision code of the device (bcdDevice). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_device_get_revision(
    const libusbp_device *,
    uint16_t * revision);

/*! Gets the serial number of the device as an ASCII-encoded string.
 *
 * On Windows, this just returns the segment of the Device Instance ID after the
 * last slash.  If the device does not have a serial number, it will return some
 * other type of identifier that contains andpersands (&).  Windows ignores
 * serial numbers with invalid characters in them.  For more information, see:
 *
 *   https://msdn.microsoft.com/en-us/library/windows/hardware/dn423379#usbsn
 *
 * On other systems, if the device does not have a serial number, then this
 * function returns an error with the code ::LIBUSBP_ERROR_NO_SERIAL_NUMBER.
 *
 * (Most applications should only call this function on specific USB devices
 * that are already known to have serial numbers, in which case the lack of a
 * serial number really does indicate a failure.)
 *
 * You should free the returned string by calling libusbp_string_free(). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_device_get_serial_number(
    const libusbp_device *,
    char ** serial_number);

/*! Gets an operating system-specific string that identifies the device.
 *
 * Note that the level of specificity provided by the ID depends on the system
 * you are on, and whether your device has a USB serial number.  As long as the
 * device remains connected to the bus, this ID is not expected to change and
 * there should be no other devices that have the same ID.  However, if the
 * device gets disconnected from the bus, it may be possible for the ID to be
 * reused by another device.
 *
 * @b Windows: This will be a device instance ID, and it will look something
 * like this:
 *
 * <pre>
 * USB\\VID_1FFB&PID_DA01\6&11A23516&18&0000
 * </pre>
 *
 * If your device has a serial number, the part after the slash will be the
 * serial number.  Otherwise, it will be a string with andpersands in it.
 *
 * @b Linux: This will be a sysfs path, and it will look like something like
 * this:
 *
 * <pre>
 * /sys/devices/pci0000:00/0000:00:06.0/usb1/1-2
 * </pre>
 *
 * <b>macOS:</b> This will be an integer from
 * IORegistryEntryGetRegistryEntryID, formatted as a lower-case hex number with
 * no leading zeros.  It will look something like this:
 *
 * <pre>
 * 10000021a
 * </pre>
 */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_device_get_os_id(
    const libusbp_device *,
    char ** id);


/** libusbp_generic_interface **************************************************/

/*! Represents a generic or vendor-defined interface of a USB device.  A null
 * libusbp_generic_interface pointer is valid and can be passed to any function
 * in this library that takes such pointers. */
typedef struct libusbp_generic_interface
               libusbp_generic_interface;

/*! Creates a generic interface object for a specified interface of the
 * specified USB device.  This function does as many checks as possible to make
 * sure that a handle to the interface could be opened, without actually opening
 * it yet.
 *
 * On all platforms, if a record of the interface cannot be found, then an error
 * is returned with the code LIBUSBP_ERROR_NOT_READY, because this could just be
 * a temporary condition that happens right after the device is plugged in.
 *
 * On Windows, the generic interface must use the WinUSB driver, or this
 * function will fail.  If it is using no driver, that could be a temporary
 * condition, and the error returned will use the LIBUSBP_ERROR_NOT_READY error
 * code.
 *
 * On Linux, if the corresponding devnode file does not exist, an error with
 * code LIBUSBP_ERROR_NOT_READY is returned.  If the interface is assigned to a
 * driver that is not "usbfs", an error is returned.
 *
 * On macOS, we do not have any additional checks beyond just making sure
 * that an entry for the interface is found.  For non-composite devices, that
 * check is deferred until a handle is opened.
 *
 * @param interface_number The lowest @a bInterfaceNumber for the interfaces in
 * the USB function you want to use.
 *
 * @param composite Should be true if the device is composite, and false
 * otherwise.
 *
 * The returned object must be freed with libusbp_generic_interface_free(). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_generic_interface_create(
    const libusbp_device *,
    uint8_t interface_number,
    bool composite,
    libusbp_generic_interface **);

/*! Frees the specified generic interface object.  Passing the NULL pointer to
 * this function is OK.  Do not free the same non-NULL pointer twice. */
LIBUSBP_API void libusbp_generic_interface_free(libusbp_generic_interface *);

/*! Makes a copy of the generic interface object.  The copy must be freed with
 *  libusbp_generic_interface_free(). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_generic_interface_copy(
    const libusbp_generic_interface * source,
    libusbp_generic_interface ** dest);

/*! Returns an operating system-specific string that can be used to uniquely
 * identify this generic interface.
 *
 * <b>Windows:</b> This will be a device instance ID specific to this interface, and
 * it will look something like this:
 *
 * <pre>
 * USB\\VID_1FFB&PID_DA01&MI_00\6&11A23516&18&0000
 * </pre>
 *
 * <b>Linux:</b> This will be a sysfs path specific to this interface, and it will
 * look like something like this:
 *
 * <pre>
 * /sys/devices/pci0000:00/0000:00:06.0/usb1/1-2/1-2:1.0
 * </pre>
 *
 * <b>macOS:</b> This will be an integer from
 * IORegistryEntryGetRegistryEntryID, formatted as a lower-case hex number with
 * no leading zeros.  It will look something like this:
 *
 * <pre>
 * 10000021a
 * </pre>
 */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_generic_interface_get_os_id(
    const libusbp_generic_interface *,
    char ** id);

/*! Returns an operating system-specific filename corresponding to this
 * interface.
 *
 * <b>Windows:</b> This will be the name of a file you can use with CreateFile to
 * access the device, and it will look something like this:
 *
 * <pre>
 * \\\\?\\usb#vid_1ffb&pid_da01&mi_00#6&11a23516&18&0000#{99c4bbb0-e925-4397-afee-981cd0702163}
 * </pre>
 *
 * <b>Linux:</b> this will return a device node file name that represents the
 * overall USB device.  It will look something like:
 *
 * <pre>
 * /dev/bus/usb/001/007
 * </pre>
 *
 * <b>macOS:</b> This will be an integer from
 * IORegistryEntryGetRegistryEntryID, formatted as a lower-case hex number with
 * no leading zeros.  It will look something like this:
 *
 * <pre>
 * 10000021a
 * </pre>
 */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_generic_interface_get_os_filename(
    const libusbp_generic_interface *,
    char ** filename);


/** libusbp_generic_handle *****************************************************/

/*! Represents a generic handle to a USB device.  This handle can be used to
 * perform operations such as control transfers and reading and writing data
 * from non-zero endpoints.
 *
 * NULL is a valid value for a libusbp_generic_handle pointer, and can be passed
 * in any functions of this library that take a libusbp_generic_handle
 * pointer. */
typedef struct libusbp_generic_handle
               libusbp_generic_handle;

/*! Opens a generic handle to the specified interface of a USB device which can
 * be used to perform USB I/O operations.
 *
 * The handle must later be closed with libusbp_generic_handle_close().
 *
 * On Windows, for devices using WinUSB, if another application has a handle
 * open already when this function is called, then this function will fail and
 * the returned error will have code ::LIBUSBP_ERROR_ACCESS_DENIED.
 *
 * On macOS, this function will set the device's configuration to 1 as a
 * side effect in case it is not already configured. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_generic_handle_open(
    const libusbp_generic_interface *,
    libusbp_generic_handle **);

/*! Closes and frees the specified generic handle.  It is OK to pass NULL to
 * this function.  Do not close the same non-NULL handle twice. All
 * ::libusbp_async_in_pipe objects created by the handle must be closed before
 * closing the handle. */
LIBUSBP_API
void libusbp_generic_handle_close(
    libusbp_generic_handle *);

/*! Creates a new asynchronous pipe object for reading data in from the device
 * on one of its bulk or interrupt IN endpoints.
 *
 * The behavior of this library is unspecified if you use both an asynchronous
 * IN pipe and synchronous reads with libusbp_read_pipe() on the same pipe of
 * the same generic handle.  One reason for that is because for WinUSB devices,
 * this function enables RAW_IO for the pipe, and it does not turn off RAW_IO
 * again after the pipe is closed.  So the behavior of libusbp_read_pipe() could
 * change depending on whether an asynchronous IN pipe has been used. */
LIBUSBP_API
libusbp_error * libusbp_generic_handle_open_async_in_pipe(
    libusbp_generic_handle *,
    uint8_t pipe_id,
    libusbp_async_in_pipe ** async_in_pipe);

/*! Sets a timeout for a particular pipe on the USB device.
 *
 * The @a pipe_id should either be 0 to specify control transfers on endpoint 0, or
 * should be a bEndpointAddress value from one of the device's endpoint
 * descriptors.  Specifying an invalid pipe might result in an error.
 *
 * The timeout value is specified in milliseconds, and a value of 0 means no
 * timeout (wait forever).  If this function is not called, the default behavior
 * of the handle is to have no timeout.
 *
 * The behavior of this function is unspecified if there is any data being
 * transferred on the pipe while this function is running.
 *
 * It is unspecified whether this timeout has an effect on asynchronous
 * transfers. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_generic_handle_set_timeout(
    libusbp_generic_handle *,
    uint8_t pipe_id,
    uint32_t timeout);

/*! Performs a synchronous (blocking) control transfer on endpoint 0.
 *
 * Under Linux, this blocking transfer unfortunately cannot be interrupted with
 * Ctrl+C.
 *
 * The @a buffer parameter should point to a buffer that is at least @a wLength
 * bytes long.
 *
 * The @a transferred pointer is optional, and is used to return the number of
 * bytes that were actually transferred.
 *
 * The direction of the transfer is determined by the @a bmRequestType parameter.
 */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_control_transfer(
    libusbp_generic_handle *,
    uint8_t bmRequestType,
    uint8_t bRequest,
    uint16_t wValue,
    uint16_t wIndex,
    void * buffer,
    uint16_t wLength,
    size_t * transferred);

/*! Performs a synchronous (blocking) write of data to a bulk or interrupt
 * endpoint.
 *
 * Under Linux, this blocking transfer unfortunately cannot be interrupted with
 * Ctrl+C.
 *
 * The @a pipe_id parameter specifies which endpoint to use.  This argument
 * should be bEndpointAddress value from one of the device's IN endpoint
 * descriptors.  (Its most significant bit must be 0.)
 *
 * The @a transferred parameter is an optional pointer to a variable that will
 * receive the number of bytes transferred. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_write_pipe(
    libusbp_generic_handle *,
    uint8_t pipe_id,
    const void * buffer,
    size_t size,
    size_t * transferred);

/*! Performs a synchronous (blocking) read of data from a bulk or interrupt
 * endpoint.
 *
 * It is best to set the buffer size to a multiple of the maximum
 * packet size of the endpoint.  Otherwise, this function might return
 * an error when the device sends more data than can fit in the
 * buffer.  This type of error is called an overflow.
 *
 * Under Linux, this blocking transfer unfortunately cannot be interrupted with
 * Ctrl+C.
 *
 * The @a pipe_id parameter specifies which endpoint to use.  This argument
 * should be bEndpointAddress value from one of the device's IN endpoint
 * descriptors.  (Its most significant bit must be 1.)
 *
 * The @a transferred parameter is an optional pointer to a variable that will
 * receive the number of bytes transferred. */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_read_pipe(
    libusbp_generic_handle *,
    uint8_t pipe_id,
    void * buffer,
    size_t size,
    size_t * transferred);

#ifdef __linux__
/*! Gets the underlying file descriptor of the generic handle.  This function is
 * only available on Linux, and is intended for advanced users.  The returned
 * file descriptor will remain open and valid as long as the handle is open and
 * has not been closed. */
LIBUSBP_API
int libusbp_generic_handle_get_fd(libusbp_generic_handle *);
#endif

#ifdef _WIN32
/*! Gets the underlying WinUSB handle for the generic handle.  This function is
 * only available on Windows, and is intended for advanced users.  The returned
 * WinUSB handle will remain open and valid as long as the generic handle is
 * open and has not been closed. */
LIBUSBP_API
HANDLE libusbp_generic_handle_get_winusb_handle(libusbp_generic_handle *);
#endif

#ifdef __APPLE__
/*! Gets the underlying IOCFPlugInInterface object representing the interface.
 * You can cast the returned pointer to a `IOCFPlugInInterface **` and then use
 * `QueryInterface` to get the corresponding `IOUSBInterfaceInterface **`.
 * There is an example of this in generic_handle_test.cpp. */
LIBUSBP_API
void ** libusbp_generic_handle_get_cf_plug_in(libusbp_generic_handle *);
#endif


/** libusbp_serial_port ********************************************************/

/*! Represents a serial port. A null libusbp_serial_port pointer is valid and
 * can be passed to any function in this library that takes such pointers. */
typedef struct libusbp_serial_port
               libusbp_serial_port;

/*! Creates a serial port object for a specified interface of the
 * specified USB device.
 *
 * On all platforms, if a record of the interface cannot be found, then an error
 * is returned with the code LIBUSBP_ERROR_NOT_READY, because this could just be
 * a temporary condition that happens right after the device is plugged in.
 *
 * On macOS, it is assumed that the interface with a @a bInterfaceNumber one
 * greater than @a interface_number is the interface that the IOSerialBSDClient
 * will attach to.  This should be true if the device implements the USB CDC ACM
 * class and has ordered its interfaces so that the control interface is right
 * before the data interface.
 *
 * @param interface_number The lowest @a bInterfaceNumber for the USB interfaces
 * that comprise the serial port.
 *
 * @param composite Should be true if the device is composite, and false
 * otherwise.
 *
 * The returned object must be freed with libusbp_generic_interface_free(). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_serial_port_create(
    const libusbp_device *,
    uint8_t interface_number,
    bool composite,
    libusbp_serial_port **);

/*! Frees the specified serial port object.  Passing the NULL pointer to
 * this function is OK.  Do not free the same non-NULL pointer twice. */
LIBUSBP_API void libusbp_serial_port_free(libusbp_serial_port *);

/*! Makes a copy of the generic interface object.  The copy must be freed with
 *  libusbp_generic_interface_free(). */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_serial_port_copy(
    const libusbp_serial_port * source,
    libusbp_serial_port ** dest);

/*! Gets the user-friendly name of the COM port
 * that could be used to open a handle.
 *
 * On Windows, this will be something like "COM12".
 *
 * On Linux, it will be something like "/dev/ttyACM0".
 *
 * On macOS, it will be something like "/dev/cu.usbmodem012345".
 * Specifically, it will be a call-out device, not a dial-in device.
 *
 * You should free the returned string by calling libusbp_string_free().
 */
LIBUSBP_API LIBUSBP_WARN_UNUSED
libusbp_error * libusbp_serial_port_get_name(
    const libusbp_serial_port *,
    char ** name);

#ifdef __cplusplus
}
#endif

