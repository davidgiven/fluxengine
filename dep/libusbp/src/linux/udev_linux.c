/* Wrapper functions for libudev that group together certain operations and
 * provide error handling. */

#include <libusbp_internal.h>

// Creates a new udev context (struct udev *).  If there is no error, the caller
// must call udev_unref at some point.
libusbp_error * udevw_create_context(struct udev ** context)
{
    assert(context != NULL);
    *context = udev_new();
    if (*context == NULL)
    {
        return error_create("Failed to create a udev context.");
    }
    return NULL;
}

// Creates a list (struct udev_enumerate *) of all devices
// that are children of a given parent device.
static libusbp_error * udevw_create_child_list(
    struct udev * udev,
    struct udev_device * parent,
    struct udev_enumerate ** list)
{
    assert(udev != NULL);
    assert(parent != NULL);
    assert(list != NULL);

    *list = NULL;

    libusbp_error * error = NULL;

    struct udev_enumerate * new_list = NULL;
    if (error == NULL)
    {
        new_list = udev_enumerate_new(udev);
        if (new_list == NULL)
        {
            error = error_create("Failed to create a udev enumeration context.");
        }
    }

    if (error == NULL)
    {
        int result = udev_enumerate_add_match_parent(new_list, parent);
        if (result != 0)
        {
            error = error_create_udev(result, "Failed to match by parent device.");
        }
    }

    if (error == NULL)
    {
        int result = udev_enumerate_scan_devices(new_list);
        if (result != 0)
        {
            error = error_create_udev(result, "Failed to scan devices.");
        }
    }

    if (error == NULL)
    {
        *list = new_list;
        new_list = NULL;
    }

    if (new_list != NULL) { udev_enumerate_unref(new_list); }
    return error;
}

// Creates a list (struct udev_enumerate *) of all devices in the "usb"
// subsystem.  This includes overall USB devices (devtype == "usb_device") and
// also interfaces (devtype == "usb_interface").  If there is no error, the
// caller must use udev_enumerate_unref at some point.
libusbp_error * udevw_create_usb_list(struct udev * udev, struct udev_enumerate ** list)
{
    assert(udev != NULL);
    assert(list != NULL);

    *list = NULL;

    libusbp_error * error = NULL;

    struct udev_enumerate * new_list = NULL;
    if (error == NULL)
    {
        new_list = udev_enumerate_new(udev);
        if (new_list == NULL)
        {
            error = error_create("Failed to create a udev enumeration context.");
        }
    }

    if (error == NULL)
    {
        int result = udev_enumerate_add_match_subsystem(new_list, "usb");
        if (result != 0)
        {
            error = error_create_udev(result, "Failed to add a subsystem match.");
        }
    }

    if (error == NULL)
    {
        int result = udev_enumerate_scan_devices(new_list);
        if (result != 0)
        {
            error = error_create_udev(result, "Failed to scan devices.");
        }
    }

    if (error == NULL)
    {
        *list = new_list;
        new_list = NULL;
    }

    if (new_list != NULL) { udev_enumerate_unref(new_list); }
    return error;
}

// Gets a udev device corresponding to the given syspath.  The syspath is the
// unique identifier that we store in order to refer to devices.
libusbp_error * udevw_get_device_from_syspath(
    struct udev * context, const char * syspath, struct udev_device ** dev)
{
    assert(context != NULL);
    assert(dev != NULL);
    assert(syspath != NULL);

    *dev = udev_device_new_from_syspath(context, syspath);
    if (*dev == NULL)
    {
        // This error can happen when the device is being unplugged.
        return error_create("Failed to get udev device from syspath: %s.", syspath);
    }

    return NULL;
}

// Get the device type of a device, typically "usb_device" or "usb_interface".
// The device type is returned as a string that you cannot modify and you do not
// have to free; it is owned by the device.
libusbp_error * udevw_get_device_type(struct udev_device * dev, const char ** devtype)
{
    assert(dev != NULL);
    assert(devtype != NULL);

    *devtype = udev_device_get_devtype(dev);
    if (*devtype == NULL)
    {
        return error_create("Failed to get device type.");
    }
    return NULL;
}

// Gets a sysattr with the specified name and parses it as a hex uint8_t.
libusbp_error * udevw_get_sysattr_uint8(
    struct udev_device * dev, const char * name, uint8_t * value)
{
    assert(dev != NULL);
    assert(name != NULL);
    assert(value != NULL);

    const char * str = udev_device_get_sysattr_value(dev, name);
    if (str == NULL)
    {
        return error_create("Device does not have sysattr %s.", name);
    }

    int result = sscanf(str, "%4hhx\n", value);
    if (result != 1)
    {
        return error_create("Failed to parse sysattr %s.", name);
    }
    return NULL;
}

// Gets a sysattr with the specified name and parses it as a hex uint16_t.
libusbp_error * udevw_get_sysattr_uint16(
    struct udev_device * dev, const char * name, uint16_t * value)
{
    assert(dev != NULL);
    assert(name != NULL);
    assert(value != NULL);

    const char * str = udev_device_get_sysattr_value(dev, name);
    if (str == NULL)
    {
        return error_create("Device does not have sysattr %s.", name);
    }

    int result = sscanf(str, "%4hx\n", value);
    if (result != 1)
    {
        return error_create("Failed to parse sysattr %s.", name);
    }
    return NULL;
}

// Gets a sysattr string and makes a copy of it.  The string must be freed with
// libusbp_string_free().  If the sysattr does not exists, returns a NULL string
// instead of raising an error.
libusbp_error * udevw_get_sysattr_if_exists_copy(
  struct udev_device * dev, const char * name, char ** value)
{
    assert(dev != NULL);
    assert(name != NULL);
    assert(value != NULL);

    *value = NULL;

    const char * str = udev_device_get_sysattr_value(dev, name);
    if (str == NULL) { return NULL; }
    return string_copy(str, value);
}

// Get the USB device of which this device is a child.  This is intended to be
// run on devices with devtype == "usb_interface" in order to get information
// about the overall USB device.
static libusbp_error * udevw_get_parent_usb_device(
    struct udev_device * dev, struct udev_device ** parent)
{
    assert(dev != NULL);
    assert(parent != NULL);
    *parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (*parent == NULL)
    {
        return error_create("Failed to get parent USB device.");
    }
    return NULL;
}

// Helper function for udevw_get_interface.
static libusbp_error * udevw_check_if_device_is_specific_interface(
    struct udev_device * dev, const char * device_syspath,
    uint8_t interface_number, bool * result)
{
    assert(dev != NULL);
    assert(device_syspath != NULL);
    assert(result != NULL);

    *result = false;

    libusbp_error * error;

    // Check that the devtype is equal to "usb_interface"
    const char * devtype;
    error = udevw_get_device_type(dev, &devtype);
    if (error != NULL) { return error; }
    if (strcmp(devtype, "usb_interface")) { return NULL; }

    // Check that bInterfaceNumber matches our interface number.
    uint8_t actual_interface_number;
    error = udevw_get_sysattr_uint8(dev, "bInterfaceNumber", &actual_interface_number);
    if (error != NULL) { return error; }
    if (actual_interface_number != interface_number) { return NULL; }

    // Get the overall USB device.
    struct udev_device * parent;
    error = udevw_get_parent_usb_device(dev, &parent);
    if (error != NULL) { return error; }

    // Check that the overall USB device is the one we are interested in.
    const char * parent_syspath = udev_device_get_syspath(parent);
    assert(parent_syspath != NULL);
    if (strcmp(parent_syspath, device_syspath)) { return NULL; }

    *result = true;
    return NULL;
}

// Finds a udev device of type "usb_interface" that is a child of the specified
// device and has the specified bInterfaceNumber.
libusbp_error * udevw_get_interface(
    struct udev * udev,
    const char * device_syspath,
    uint8_t interface_number,
    struct udev_device ** device)
{
    assert(udev != NULL);
    assert(device_syspath != NULL);
    assert(device != NULL);

    *device = NULL;

    libusbp_error * error = NULL;

    struct udev_enumerate * list = NULL;
    if (error == NULL)
    {
        // Note: It would probably be better to create a list using
        // udev_enumerate_add_match_parent so the list is smaller.
        error = udevw_create_usb_list(udev, &list);
    }

    // Loop over the list to find the device.
    struct udev_device * found_device = NULL;
    if (error == NULL)
    {
        struct udev_list_entry * first_entry = udev_enumerate_get_list_entry(list);
        struct udev_list_entry * list_entry;
        udev_list_entry_foreach(list_entry, first_entry)
        {
            const char * path = udev_list_entry_get_name(list_entry);
            assert(path != NULL);

            // Get the udev device.
            struct udev_device * dev = NULL;
            error = udevw_get_device_from_syspath(udev, path, &dev);
            if (error != NULL) { break; }

            // See if it is the right device.
            bool correct_device = false;
            error = udevw_check_if_device_is_specific_interface(dev, device_syspath,
                interface_number, &correct_device);
            if (error != NULL)
            {
                udev_device_unref(dev);
                break;
            }

            // If it is the right device, stop looping.
            if (correct_device)
            {
                found_device = dev;
                break;
            }

            udev_device_unref(dev);
        }
    }

    // Make sure we found the device.
    if (error == NULL && found_device == NULL)
    {
        // We did not find it.  Maybe the interface is just not ready yet and it
        // would be ready in a few milliseconds.  (Note: We have not seen this
        // happen, but adding the error code here makes the behavior consistent
        // with how the Windows code behaves.)
        error = error_create("Could not find interface %d.", interface_number);
        error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
    }

    // Pass the device back to the caller.
    if (error == NULL)
    {
        *device = found_device;
    }

    if (list != NULL) { udev_enumerate_unref(list); }
    return error;
}

// Finds a udev device of type "usb_interface" that is a child of the specified
// device and has the specified bInterfaceNumber.
libusbp_error * udevw_get_tty(
    struct udev * udev,
    struct udev_device * parent,
    struct udev_device ** device)
{
    assert(udev != NULL);
    assert(parent != NULL);
    assert(device != NULL);

    *device = NULL;

    libusbp_error * error = NULL;

    struct udev_enumerate * list = NULL;
    if (error == NULL)
    {
        error = udevw_create_child_list(udev, parent, &list);
    }

    // Loop over the list to find the device.
    struct udev_device * found_device = NULL;
    if (error == NULL)
    {
        struct udev_list_entry * first_entry = udev_enumerate_get_list_entry(list);
        struct udev_list_entry * list_entry;
        udev_list_entry_foreach(list_entry, first_entry)
        {
            const char * path = udev_list_entry_get_name(list_entry);
            assert(path != NULL);

            // Get the udev device.
            struct udev_device * dev = NULL;
            error = udevw_get_device_from_syspath(udev, path, &dev);
            if (error != NULL) { break; }

            // See if it is in the "tty" subsystem.
            bool correct_device = false;
            const char * subsystem = udev_device_get_subsystem(dev);
            if (subsystem != NULL && 0 == strcmp(subsystem, "tty"))
            {
                correct_device = true;
            }

            // If it is the right device, stop looping.
            if (correct_device)
            {
                found_device = dev;
                break;
            }

            udev_device_unref(dev);
        }
    }

    // Make sure we found the device.
    if (error == NULL && found_device == NULL)
    {
        // We did not find it.  Maybe the interface is just not ready yet and it
        // would be ready in a few milliseconds.  (Note: We have not seen this
        // happen, but adding the error code here makes the behavior consistent
        // with how the Windows code behaves.)
        error = error_create("Could not find tty device.");
        error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
    }

    // Pass the device back to the caller.
    if (error == NULL)
    {
        *device = found_device;
    }

    if (list != NULL) { udev_enumerate_unref(list); }
    return error;
}

// Gets the devnode path of the specified udev device.  The returned string is
// owned by the device.
libusbp_error * udevw_get_syspath(struct udev_device * device, const char ** syspath)
{
    assert(device != NULL);
    assert(syspath != NULL);

    *syspath = udev_device_get_syspath(device);
    assert(*syspath != NULL);
    return NULL;
}

// Gets the syspath of the specified udev device.  The returned string must
// be freed with libusbp_string_free.
libusbp_error * udevw_get_syspath_copy(struct udev_device * device, char ** syspath)
{
    assert(device != NULL);
    assert(syspath != NULL);

    *syspath = NULL;

    libusbp_error * error = NULL;

    const char * tmp = NULL;
    if (error == NULL)
    {
        error = udevw_get_syspath(device, &tmp);
    }

    if (error == NULL)
    {
        error = string_copy(tmp, syspath);
    }
    return error;
}

// Gets the devnode path of the specified udev device.  The returned string is
// owned by the device.
libusbp_error * udevw_get_devnode(struct udev_device * device, const char ** devnode)
{
    assert(device != NULL);
    assert(devnode != NULL);

    *devnode = udev_device_get_devnode(device);
    if (*devnode == NULL)
    {
        return error_create("No device node exists.");
    }
    return NULL;
}


// Gets the devnode of the specified udev device.  The returned string must
// be freed with libusbp_string_free.
libusbp_error * udevw_get_devnode_copy(struct udev_device * device, char ** devnode)
{
    assert(device != NULL);
    assert(devnode != NULL);

    *devnode = NULL;

    libusbp_error * error = NULL;

    const char * tmp = NULL;
    if (error == NULL)
    {
        error = udevw_get_devnode(device, &tmp);
    }

    if (error == NULL)
    {
        error = string_copy(tmp, devnode);
    }
    return error;
}

// Takes a syspath as input and gets the corresponding devpath.  The returned
// string must be freed with libusbp_string_free if there were no errors.
libusbp_error * udevw_get_devnode_copy_from_syspath(const char * syspath, char ** devnode)
{
    assert(syspath != NULL);
    assert(devnode != NULL);

    *devnode = NULL;

    libusbp_error * error = NULL;

    struct udev * context = NULL;
    if (error == NULL)
    {
        error = udevw_create_context(&context);
    }

    struct udev_device * device = NULL;
    if (error == NULL)
    {
        error = udevw_get_device_from_syspath(context, syspath, &device);
    }

    if (error == NULL)
    {
        error = udevw_get_devnode_copy(device, devnode);
    }

    if (device != NULL) { udev_device_unref(device); }
    if (context != NULL) { udev_unref(context); }
    return error;
}
