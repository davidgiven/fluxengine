#ifdef LIBUSBP_DROP_IN
#include "libusbp.h"
#else
#pragma once
#include <libusbp_config.h>
#include <libusbp.h>
#if BUILD_SYSTEM_LIBUSBP_VERSION_MAJOR != LIBUSBP_VERSION_MAJOR
#error Major version in libusbp.h disagrees with build system.
#endif
#endif

// Don't warn about zero-length format strings, which we sometimes use when
// constructing error objects.
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat-zero-length"
#endif

// Silence some warnings from the Microsoft C Compiler.
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define strdup _strdup
#endif

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <devpropdef.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <usbiodef.h>
#include <usbioctl.h>
#include <stringapiset.h>
#include <winusb.h>
#endif

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <libudev.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/ch9.h>
#include <sys/ioctl.h>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <mach/error.h>
#include <mach/mach_error.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFBundle.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>
#endif

// For debug builds (NDEBUG undefined), functions and variables declared with
// LIBUSBP_TEST_API will be exported in the library so that they can be unit
// tested, but they are not part of the public API and they might change at any
// time.  Those symbols can cause name collisions with symbols defined by the
// user, so this type of build is not suitable for making a general release.
#ifdef NDEBUG
#define LIBUSBP_TEST_API
#else
#define LIBUSBP_TEST_API LIBUSBP_API
#endif

#ifdef _MSC_VER
#define LIBUSBP_PRINTF(f, a)
#else
#define LIBUSBP_PRINTF(f, a) __attribute__((format (printf, f, a)))
#endif

// Suppresses unused parameter warnings.
#define LIBUSBP_UNUSED(param_name) (void)param_name;

#define MAX_ENDPOINT_NUMBER 15

typedef struct libusbp_setup_packet
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} libusbp_setup_packet;

LIBUSBP_TEST_API extern libusbp_error error_no_memory;

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED libusbp_error * error_create(
    const char * format, ...) LIBUSBP_PRINTF(1, 2);

LIBUSBP_WARN_UNUSED libusbp_error * error_add_v(
    libusbp_error * error, const char * format, va_list ap);

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED libusbp_error * error_add(
    libusbp_error * error, const char * format, ...)
    LIBUSBP_PRINTF(2, 3);

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED libusbp_error * error_add_code(
    libusbp_error * error, uint32_t code);

LIBUSBP_WARN_UNUSED libusbp_error * string_copy(
    const char * input_string,
    char ** output_string);

LIBUSBP_WARN_UNUSED
libusbp_error * check_pipe_id(uint8_t pipe_id);

LIBUSBP_WARN_UNUSED
libusbp_error * check_pipe_id_in(uint8_t pipe_id);

LIBUSBP_WARN_UNUSED
libusbp_error * check_pipe_id_out(uint8_t pipe_id);

LIBUSBP_WARN_UNUSED
libusbp_error * device_list_create(libusbp_device *** device_list);

LIBUSBP_WARN_UNUSED
libusbp_error * device_list_append(libusbp_device *** device_list,
    size_t * device_count, libusbp_device * device);

void free_devices_and_list(libusbp_device ** device_list);

typedef struct async_in_transfer
               async_in_transfer;

void async_in_transfer_handle_completion(async_in_transfer *);

void async_in_transfer_free(async_in_transfer * transfer);

libusbp_error * async_in_transfer_create(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    size_t transfer_size,
    async_in_transfer ** transfer);

LIBUSBP_WARN_UNUSED
libusbp_error * async_in_pipe_create(
    libusbp_generic_handle * handle,
    uint8_t pipe_id,
    libusbp_async_in_pipe ** pipe);

LIBUSBP_WARN_UNUSED
libusbp_error * async_in_pipe_setup(libusbp_generic_handle * gh, uint8_t pipe_id);

void async_in_transfer_submit(async_in_transfer * transfer);

LIBUSBP_WARN_UNUSED
libusbp_error * async_in_transfer_get_results(async_in_transfer * transfer,
    void * buffer, size_t * transferred, libusbp_error ** transfer_error);

LIBUSBP_WARN_UNUSED
libusbp_error * async_in_transfer_cancel(async_in_transfer * transfer);

bool async_in_transfer_pending(async_in_transfer * transfer);

LIBUSBP_WARN_UNUSED
libusbp_error * generic_handle_events(libusbp_generic_handle * handle);

#ifdef _WIN32

LIBUSBP_WARN_UNUSED libusbp_error * create_device(HDEVINFO list,
    PSP_DEVINFO_DATA info, libusbp_device ** device);

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED LIBUSBP_PRINTF(1, 2)
libusbp_error * error_create_winapi(const char * format, ...);

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED LIBUSBP_PRINTF(1, 2)
libusbp_error * error_create_overlapped(const char * format, ...);

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED LIBUSBP_PRINTF(2, 3)
libusbp_error * error_create_cr(CONFIGRET, const char * format, ...);

LIBUSBP_WARN_UNUSED libusbp_error * get_interface(
    const char * device_instance_id,
    uint8_t interface_number,
    bool composite,
    HDEVINFO * list,
    PSP_DEVINFO_DATA info);

LIBUSBP_WARN_UNUSED libusbp_error * create_id_string(
    HDEVINFO list, PSP_DEVINFO_DATA info, char ** id);

LIBUSBP_WARN_UNUSED libusbp_error * get_filename_from_devinst_and_guid(
    DEVINST devinst,
    const GUID * guid,
    char ** filename);

#endif

#ifdef __linux__

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED LIBUSBP_PRINTF(1, 2)
libusbp_error * error_create_errno(const char * format, ...);

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED LIBUSBP_PRINTF(2, 3)
libusbp_error * error_create_udev(int error_code, const char * format, ...);

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED
libusbp_error * error_from_urb_status(struct usbdevfs_urb * urb);

LIBUSBP_WARN_UNUSED
libusbp_error * device_create(struct udev_device * dev, libusbp_device ** device);

LIBUSBP_WARN_UNUSED
libusbp_error * generic_interface_get_device_copy(
    const libusbp_generic_interface * gi, libusbp_device ** device);

/** udevw **********************************************************************/

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_create_context(struct udev **);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_create_usb_list(struct udev * context, struct udev_enumerate ** list);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_device_from_syspath(struct udev *,
    const char * syspath, struct udev_device ** dev);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_device_type(struct udev_device *, const char ** devtype);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_sysattr_uint8(
    struct udev_device * dev, const char * name, uint8_t * value);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_sysattr_uint16(
    struct udev_device * dev, const char * name, uint16_t * value);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_sysattr_if_exists_copy(
  struct udev_device * dev, const char * name, char ** value);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_interface(
    struct udev * udev,
    const char * device_syspath,
    uint8_t interface_number,
    struct udev_device ** device);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_tty(
    struct udev * udev,
    struct udev_device * parent,
    struct udev_device ** device);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_syspath(struct udev_device * device, const char ** syspath);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_syspath_copy(struct udev_device * device, char ** devnode);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_devnode(struct udev_device * device, const char ** devnode);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_devnode_copy(struct udev_device * device, char ** devnode);

LIBUSBP_WARN_UNUSED
libusbp_error * udevw_get_devnode_copy_from_syspath(const char * syspath, char ** devnode);


/** usbfd **********************************************************************/

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_check_existence(const char * filename);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_open(const char * path, int * fd);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_get_device_descriptor(int fd, struct usb_device_descriptor * desc);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_control_transfer(int fd, libusbp_setup_packet setup,
    uint32_t timeout, void * data, size_t * transferred);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_control_transfer_async(int fd, void * combined_buffer,
    size_t size, uint32_t timeout, void * user_context);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_bulk_or_interrupt_transfer(int fd, uint8_t pipe, uint32_t timeout,
    void * buffer, size_t size, size_t * transferred);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_submit_urb(int fd, struct usbdevfs_urb * urb);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_reap_urb(int fd, struct usbdevfs_urb ** urb);

LIBUSBP_WARN_UNUSED
libusbp_error * usbfd_discard_urb(int fd, struct usbdevfs_urb * urb);

#endif

#ifdef __APPLE__

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED LIBUSBP_PRINTF(2, 3)
libusbp_error * error_create_mach(kern_return_t error_code, const char * format, ...);

LIBUSBP_WARN_UNUSED
libusbp_error * create_device(io_service_t service, libusbp_device ** device);

uint64_t device_get_id(const libusbp_device * device);

bool generic_interface_get_interface_id(const libusbp_generic_interface * gi, uint64_t * id);
uint64_t generic_interface_get_device_id(const libusbp_generic_interface * gi);
uint8_t generic_interface_get_interface_number(const libusbp_generic_interface * gi);

IOUSBInterfaceInterface182 ** generic_handle_get_ioh(const libusbp_generic_handle * handle);

uint8_t generic_handle_get_pipe_index(const libusbp_generic_handle * handle, uint8_t pipe_id);

LIBUSBP_WARN_UNUSED
libusbp_error * iokit_id_to_string(uint64_t id, char ** str);

LIBUSBP_WARN_UNUSED
libusbp_error * service_get_from_id(uint64_t id, io_service_t *);

LIBUSBP_WARN_UNUSED
libusbp_error * service_get_usb_interface(io_service_t service,
    uint8_t interface_number, io_service_t * interface_service);

LIBUSBP_WARN_UNUSED
libusbp_error * service_get_child_by_class(io_service_t service,
    const char * class_name, io_service_t * interface_service);

LIBUSBP_WARN_UNUSED
libusbp_error * service_to_interface(io_service_t, CFUUIDRef pluginType, REFIID rid,
    void **, IOCFPlugInInterface ***);

LIBUSBP_WARN_UNUSED
libusbp_error * get_id(io_registry_entry_t entry, uint64_t * id);

LIBUSBP_WARN_UNUSED
libusbp_error * get_string(io_registry_entry_t entry,
    CFStringRef name, char ** value);

LIBUSBP_WARN_UNUSED
libusbp_error * get_int32(io_registry_entry_t entry,
    CFStringRef name, int32_t * value);

LIBUSBP_WARN_UNUSED
libusbp_error * get_uint16(io_registry_entry_t entry,
    CFStringRef name, uint16_t * value);

#endif

#if defined(_WIN32) || defined(__APPLE__)

LIBUSBP_TEST_API LIBUSBP_WARN_UNUSED LIBUSBP_PRINTF(2, 3)
libusbp_error * error_create_hr(HRESULT, const char * format, ...);

#endif
