#include <libusbp_internal.h>

struct libusbp_device
{
    uint64_t id;
    uint16_t product_id;
    uint16_t vendor_id;
    uint16_t revision;
    char * serial_number;
};

static libusbp_error * device_allocate(libusbp_device ** device)
{
    assert(device != NULL);
    *device = calloc(1, sizeof(libusbp_device));
    if (*device == NULL)
    {
        return &error_no_memory;
    }
    return NULL;
}

libusbp_error * create_device(io_service_t service, libusbp_device ** device)
{
    assert(service != MACH_PORT_NULL);
    assert(device != NULL);

    libusbp_error * error = NULL;

    // Allocate the device.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        error = device_allocate(&new_device);
    }

    // Get the numeric IDs.
    if (error == NULL)
    {
        error = get_uint16(service, CFSTR(kUSBVendorID), &new_device->vendor_id);
    }
    if (error == NULL)
    {
        error = get_uint16(service, CFSTR(kUSBProductID), &new_device->product_id);
    }
    if (error == NULL)
    {
        error = get_uint16(service, CFSTR(kUSBDeviceReleaseNumber), &new_device->revision);
    }

    // Get the serial number.
    if (error == NULL)
    {
        error = get_string(service, CFSTR(kUSBSerialNumberString), &new_device->serial_number);
    }

    // Get the ID.
    if (error == NULL)
    {
        error = get_id(service, &new_device->id);
    }

    // Pass the device to the caller.
    if (error == NULL)
    {
        *device = new_device;
        new_device = NULL;
    }

    libusbp_device_free(new_device);
    return error;
}

libusbp_error * libusbp_device_copy(const libusbp_device * source, libusbp_device ** dest)
{
    if (dest == NULL)
    {
        return error_create("Device output pointer is null.");
    }

    *dest = NULL;

    if (source == NULL)
    {
        return NULL;
    }

    libusbp_error * error = NULL;

    // Allocate the device.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        error = device_allocate(&new_device);
    }

    // Copy the simple fields, while leaving the pointers owned by the
    // device NULL so that libusbp_device_free is still OK to call.
    if (error == NULL)
    {
        memcpy(new_device, source, sizeof(libusbp_device));
        new_device->serial_number = NULL;
    }

    // Copy the serial number.
    if (error == NULL && source->serial_number != NULL)
    {
        error = string_copy(source->serial_number, &new_device->serial_number);
    }

    // Pass the device to the caller.
    if (error == NULL)
    {
        *dest = new_device;
        new_device = NULL;
    }

    libusbp_device_free(new_device);
    return error;
}

void libusbp_device_free(libusbp_device * device)
{
    if (device != NULL)
    {
        libusbp_string_free(device->serial_number);
        free(device);
    }
}

uint64_t device_get_id(const libusbp_device * device)
{
    assert(device != NULL);
    return device->id;
}

libusbp_error * libusbp_device_get_vendor_id(
    const libusbp_device * device,
    uint16_t * vendor_id)
{
    if (vendor_id == NULL)
    {
        return error_create("Vendor ID output pointer is null.");
    }

    *vendor_id = 0;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    *vendor_id = device->vendor_id;
    return NULL;
}


libusbp_error * libusbp_device_get_product_id(
    const libusbp_device * device,
    uint16_t * product_id)
{
    if (product_id == NULL)
    {
        return error_create("Product ID output pointer is null.");
    }

    *product_id = 0;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    *product_id = device->product_id;
    return NULL;
}

libusbp_error * libusbp_device_get_revision(
    const libusbp_device * device,
    uint16_t * revision)
{
    if (revision == NULL)
    {
        return error_create("Device revision output pointer is null.");
    }

    *revision = 0;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    *revision = device->revision;
    return NULL;
}

libusbp_error * libusbp_device_get_serial_number(
    const libusbp_device * device,
    char ** serial_number)
{
    if (serial_number == NULL)
    {
        return error_create("Serial number output pointer is null.");
    }

    *serial_number = NULL;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    if (device->serial_number == NULL)
    {
        libusbp_error * error = error_create("Device does not have a serial number.");
        error = error_add_code(error, LIBUSBP_ERROR_NO_SERIAL_NUMBER);
        return error;
    }

    return string_copy(device->serial_number, serial_number);
}

libusbp_error * libusbp_device_get_os_id(
    const libusbp_device * device,
    char ** id)
{
    if (id == NULL)
    {
        return error_create("Device OS ID output pointer is null.");
    }

    *id = NULL;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    return iokit_id_to_string(device->id, id);
}
