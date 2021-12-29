#include <libusbp_internal.h>

struct libusbp_device
{
    char * syspath;
    char * serial_number;  // may be NULL
    uint16_t product_id;
    uint16_t vendor_id;
    uint16_t revision;
};

libusbp_error * device_create(struct udev_device * dev, libusbp_device ** device)
{
    assert(dev != NULL);
    assert(device != NULL);

    libusbp_error * error = NULL;

    // Allocate memory for the device.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        new_device = malloc(sizeof(libusbp_device));
        if (new_device == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Get the syspath.
    char * new_syspath = NULL;
    if (error == NULL)
    {
        error = udevw_get_syspath_copy(dev, &new_syspath);
    }

    // Get the vendor ID.
    uint16_t vendor_id;
    if (error == NULL)
    {
        error = udevw_get_sysattr_uint16(dev, "idVendor", &vendor_id);
    }

    // Get the product ID.
    uint16_t product_id;
    if (error == NULL)
    {
        error = udevw_get_sysattr_uint16(dev, "idProduct", &product_id);
    }

    // Get the revision.
    uint16_t revision;
    if (error == NULL)
    {
        error = udevw_get_sysattr_uint16(dev, "bcdDevice", &revision);
    }

    // Get the serial number.
    char * new_serial_number = NULL;
    if (error == NULL)
    {
        error = udevw_get_sysattr_if_exists_copy(dev, "serial", &new_serial_number);
    }

    // Populate the new device and give it to the caller.
    if (error == NULL)
    {
        new_device->syspath = new_syspath;
        new_device->serial_number = new_serial_number;
        new_device->vendor_id = vendor_id;
        new_device->product_id = product_id;
        new_device->revision = revision;
        *device = new_device;

        new_syspath = NULL;
        new_serial_number = NULL;
        new_device = NULL;
    }

    free(new_serial_number);
    free(new_syspath);
    free(new_device);
    return error;
}

void libusbp_device_free(libusbp_device * device)
{
    if (device != NULL)
    {
        free(device->serial_number);
        free(device->syspath);
        free(device);
    }
}

libusbp_error * libusbp_device_copy(
    const libusbp_device * source, libusbp_device ** dest)
{
    if (dest == NULL)
    {
        return error_create("Device output pointer is null.");
    }

    *dest = NULL;

    if (source == NULL) { return NULL; }

    libusbp_error * error = NULL;

    // Allocate memory for the device.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        new_device = malloc(sizeof(libusbp_device));
        if (new_device == NULL) { error = &error_no_memory; }
    }

    // Copy the syspath.
    char * new_syspath = NULL;
    if (error == NULL)
    {
        assert(source->syspath != NULL);
        error = string_copy(source->syspath, &new_syspath);
    }

    // Copy the serial number if it is not NULL.
    char * new_serial_number = NULL;
    if (error == NULL && source->serial_number != NULL)
    {
        error = string_copy(source->serial_number, &new_serial_number);
    }

    // Assemble the new device and give it to the caller.
    if (error == NULL)
    {
        memcpy(new_device, source, sizeof(libusbp_device));
        new_device->syspath = new_syspath;
        new_device->serial_number = new_serial_number;
        *dest = new_device;

        new_device = NULL;
        new_syspath = NULL;
        new_serial_number = NULL;
    }

    // Clean up and return.
    free(new_device);
    free(new_syspath);
    free(new_serial_number);
    return error;
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

    return string_copy(device->syspath, id);
}
