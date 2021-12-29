#include <libusbp_internal.h>

// Adds a device to the list, while maintaining the loop invariants for the loop
// in libusbp_list_connected_devices.
static libusbp_error * add_udev_device_to_list(
    struct udev_device * dev, libusbp_device *** device_list, size_t * device_count)
{
    assert(dev != NULL);
    assert(device_list != NULL);
    assert(device_count != NULL);

    libusbp_error * error = NULL;

    // Create a new device.
    libusbp_device * new_device;
    error = device_create(dev, &new_device);
    if (error != NULL) { return error; }

    // Add the device to the list.
    error = device_list_append(device_list, device_count, new_device);
    if (error != NULL)
    {
        libusbp_device_free(new_device);
        return error;
    }
    return NULL;
}

static libusbp_error * add_udev_device_to_list_if_needed(
    struct udev * udev, const char * syspath,
    libusbp_device *** device_list, size_t * device_count
)
{
    assert(udev != NULL);
    assert(syspath != NULL);
    assert(device_list != NULL);
    assert(device_count != NULL);

    libusbp_error * error = NULL;

    // Get the udev device.
    struct udev_device * dev = NULL;
    if (error == NULL)
    {
        error = udevw_get_device_from_syspath(udev, syspath, &dev);
    }

    // Get the devtype, which is typically usb_device or usb_interface.
    const char * devtype = NULL;
    if (error == NULL)
    {
        error = udevw_get_device_type(dev, &devtype);
    }

    bool skip = false;

    // Skip the interfaces, just add the actual USB devices.
    if (error == NULL && !skip && strcmp(devtype, "usb_device") != 0)
    {
        skip = true;
    }

    // Skip devices that have not been initialized yet, because the udev rules
    // might not be fully applied.  They might have the wrong permissions.
    if (error == NULL && !skip && !udev_device_get_is_initialized(dev))
    {
        skip = true;
    }

    if (error == NULL && !skip)
    {
        // This is a USB device, so we do want to add it to the list.
        error = add_udev_device_to_list(dev, device_list, device_count);
    }

    if (dev != NULL)
    {
        udev_device_unref(dev);
    }
    return error;
}

libusbp_error * libusbp_list_connected_devices(
  libusbp_device *** device_list, size_t * device_count)
{
    if (device_count != NULL)
    {
        *device_count = 0;
    }

    if (device_list == NULL)
    {
        return error_create("Device list output pointer is null.");
    }

    libusbp_error * error = NULL;

    // Create a udev context.
    struct udev * udev = NULL;
    if (error == NULL)
    {
        error = udevw_create_context(&udev);
    }

    // Create a list of USB devices and interfaces.
	struct udev_enumerate * enumerate = NULL;
    if (error == NULL)
    {
        error = udevw_create_usb_list(udev, &enumerate);
    }

    // Allocate a new list.
    libusbp_device ** new_list = NULL;
    size_t count = 0;
    if (error == NULL)
    {
        error = device_list_create(&new_list);
    }

    // Iterate over the devices and add them to the list.
    if (error == NULL)
    {
        struct udev_list_entry * first_entry = udev_enumerate_get_list_entry(enumerate);
        struct udev_list_entry * list_entry;
        udev_list_entry_foreach(list_entry, first_entry)
        {
            const char * path = udev_list_entry_get_name(list_entry);
            assert(path != NULL);

            error = add_udev_device_to_list_if_needed(udev, path, &new_list, &count);
            if (error != NULL)
            {
                // Something went wrong when getting information about the
                // device.  When unplugging a device, we often see
                // udev_device_new_from_syspath return NULL, which could cause
                // this error.  To make the library more robust and usable, we
                // ignore this error and continue.
                #ifdef LIBUSBP_LOG
                fprintf(stderr, "Problem adding device to list: %s\n",
                    libusbp_error_get_message(error));
                #endif
                libusbp_error_free(error);
                error = NULL;
            }
        }
    }

    // Pass the list and the count to the caller.
    if (error == NULL)
    {
        *device_list = new_list;
        new_list = NULL;

        if (device_count != NULL)
        {
            *device_count = count;
        }
    }

    // Clean up everything we used.
    if (enumerate != NULL) { udev_enumerate_unref(enumerate); }
    if (udev != NULL) { udev_unref(udev); }
    free_devices_and_list(new_list);
    return error;
}
