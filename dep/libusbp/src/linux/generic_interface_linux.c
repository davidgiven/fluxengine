#include <libusbp_internal.h>

struct libusbp_generic_interface
{
    libusbp_device * device;

    uint8_t interface_number;

    // A sysfs path like "/sys/devices/pci0000:00/0000:00:06.0/usb1/1-2/1-2:1.0"
    char * syspath;

    // A filename like "/dev/bus/usb/001/007"
    char * filename;
};

libusbp_error * check_driver_installation(struct udev_device * device)
{
    assert(device != NULL);
    const char * driver_name = udev_device_get_driver(device);
    if (driver_name != NULL && strcmp(driver_name, "usbfs") && strcmp(driver_name, "cp210x"))
    {
        return error_create("Device is attached to an incorrect driver: %s.", driver_name);
    }
    return NULL;
}

libusbp_error * libusbp_generic_interface_create(
    const libusbp_device * device,
    uint8_t interface_number,
    bool composite __attribute__((unused)),
    libusbp_generic_interface ** gi)
{
    if (gi == NULL)
    {
        return error_create("Generic interface output pointer is null.");
    }

    *gi = NULL;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    libusbp_error * error = NULL;

    // Allocate memory for the interface.
    libusbp_generic_interface * new_gi = NULL;
    if (error == NULL)
    {
        new_gi = malloc(sizeof(libusbp_generic_interface));
        if (new_gi == NULL) { error = &error_no_memory; }
    }

    // Make a copy of the device (since the original device could be freed
    // before this generic interface is freed.)
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        error = libusbp_device_copy(device, &new_device);
    }

    // Get the syspath of the device.
    char * new_device_syspath = NULL;
    if (error == NULL)
    {
        error = libusbp_device_get_os_id(new_device, &new_device_syspath);
    }

    // Get a udev context.
    struct udev * new_udev = NULL;
    if (error == NULL)
    {
        error = udevw_create_context(&new_udev);
    }

    // Get the device for the interface.
    struct udev_device * new_dev = NULL;
    if (error == NULL)
    {
        error = udevw_get_interface(new_udev, new_device_syspath,
            interface_number, &new_dev);
    }

    // Make sure it is not attached to a kernel driver.
    // Note: This step might be inappropriate, since libusbp can operate
    // on some devices that are attached to a kernel driver, like the cp210x
    // driver.
    if (error == NULL)
    {
        error = check_driver_installation(new_dev);
    }

    // Get the syspath of the interface.
    char * new_interface_syspath = NULL;
    if (error == NULL)
    {
        error = udevw_get_syspath_copy(new_dev, &new_interface_syspath);
    }

    // Get the filename.
    char * new_filename = NULL;
    if (error == NULL)
    {
        error = udevw_get_devnode_copy_from_syspath(new_device_syspath, &new_filename);
    }

    // Check that the file exists yet, but don't check to see if we have permission
    // to open it.  This behavior was chosen to be as similar to the Windows
    // behavior as possible.
    if (error == NULL)
    {
        error = usbfd_check_existence(new_filename);
    }

    // Assemble the new generic interface and pass it to the caller.
    if (error == NULL)
    {
        new_gi->interface_number = interface_number;

        new_gi->device = new_device;
        new_device = NULL;

        new_gi->syspath = new_interface_syspath;
        new_interface_syspath = NULL;

        new_gi->filename = new_filename;
        new_filename = NULL;

        *gi = new_gi;
        new_gi = NULL;
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to initialize generic interface.");
    }

    libusbp_string_free(new_filename);
    libusbp_string_free(new_interface_syspath);
    libusbp_string_free(new_device_syspath);
    libusbp_device_free(new_device);
    free(new_gi);
    if (new_dev != NULL) { udev_device_unref(new_dev); }
    if (new_udev != NULL) { udev_unref(new_udev); }
    return error;
}

void libusbp_generic_interface_free(libusbp_generic_interface * gi)
{
    if (gi != NULL)
    {
        libusbp_device_free(gi->device);
        libusbp_string_free(gi->syspath);
        libusbp_string_free(gi->filename);
        free(gi);
    }
}

libusbp_error * libusbp_generic_interface_copy(
    const libusbp_generic_interface * source,
    libusbp_generic_interface ** dest)
{
    if (dest == NULL)
    {
        return error_create("Generic interface output pointer is null.");
    }

    *dest = NULL;

    if (source == NULL)
    {
        return NULL;
    }

    libusbp_error * error = NULL;

    // Allocate memory for the generic interface.
    libusbp_generic_interface * new_gi = NULL;
    if (error == NULL)
    {
        new_gi = malloc(sizeof(libusbp_generic_interface));
        if (new_gi == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Copy the device.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        error = libusbp_device_copy(source->device, &new_device);
    }

    // Copy the syspath.
    char * new_syspath = NULL;
    if (error == NULL)
    {
        error = string_copy(source->syspath, &new_syspath);
    }

    // Copy the filename.
    char * new_filename = NULL;
    if (error == NULL)
    {
        error = string_copy(source->filename, &new_filename);
    }

    // Assemble the new interface and return it to the caller.
    if (error == NULL)
    {
        memcpy(new_gi, source, sizeof(libusbp_generic_interface));
        new_gi->device = new_device;
        new_gi->syspath = new_syspath;
        new_gi->filename = new_filename;
        *dest = new_gi;

        new_filename = NULL;
        new_syspath = NULL;
        new_device = NULL;
        new_gi = NULL;
    }

    libusbp_string_free(new_filename);
    libusbp_string_free(new_syspath);
    libusbp_device_free(new_device);
    free(new_gi);
    return NULL;
}

libusbp_error * libusbp_generic_interface_get_os_id(
    const libusbp_generic_interface * gi, char ** id)
{
    if (id == NULL)
    {
        return error_create("String output pointer is null.");
    }

    *id = NULL;

    if (gi == NULL)
    {
        return error_create("Generic interface is null.");
    }

    assert(gi->syspath != NULL);
    return string_copy(gi->syspath, id);
}

libusbp_error * libusbp_generic_interface_get_os_filename(
    const libusbp_generic_interface * gi, char ** filename)
{
    if (filename == NULL)
    {
        return error_create("String output pointer is null.");
    }

    *filename = NULL;

    if (gi == NULL)
    {
        return error_create("Generic interface is null.");
    }

    assert(gi->filename != NULL);
    return string_copy(gi->filename, filename);
}

libusbp_error * generic_interface_get_device_copy(
    const libusbp_generic_interface * gi, libusbp_device ** device)
{
    assert(gi != NULL);
    assert(device != NULL);
    return libusbp_device_copy(gi->device, device);
}
