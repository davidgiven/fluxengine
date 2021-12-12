#include <libusbp_internal.h>

libusbp_error * iokit_id_to_string(uint64_t id, char ** str)
{
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%" PRIx64, id);
    return string_copy(buffer, str);
}

libusbp_error * service_get_from_id(uint64_t id, io_service_t * service)
{
    assert(service != NULL);

    // Create a dictionary specifying this ID.  This dictionary will be
    // CFReleased by IOServiceGetMatchingService.
    CFMutableDictionaryRef dict = IORegistryEntryIDMatching(id);
    if (dict == NULL)
    {
        return error_create("Failed to create a dictionary matching the ID.");
    }

    *service = IOServiceGetMatchingService(kIOMasterPortDefault, dict);
    if (*service == MACH_PORT_NULL)
    {
        return error_create("Failed to find service with ID 0x%" PRIx64 ".", id);
    }

    return NULL;
}

// Takes a service representing a physical, composite USB device.
// Returns a service representing the specified USB interface of the device.
// The returned service will conform to the class IOUSBInterface.
libusbp_error * service_get_usb_interface(io_service_t service,
    uint8_t interface_number, io_service_t * interface_service)
{
    assert(service != MACH_PORT_NULL);
    assert(interface_service != NULL);

    *interface_service = MACH_PORT_NULL;

    libusbp_error * error = NULL;

    io_iterator_t iterator = MACH_PORT_NULL;
    kern_return_t result = IORegistryEntryGetChildIterator(
        service, kIOServicePlane, &iterator);

    if (result != KERN_SUCCESS)
    {
        error = error_create_mach(result, "Failed to get child iterator.");
    }

    // Loop through the devices to find the right one.
    while (error == NULL)
    {
        io_service_t candidate = IOIteratorNext(iterator);
        if (candidate == MACH_PORT_NULL) { break; }

        // Filter out candidates that are not of class IOUSBInterface.
        bool conforms = (bool) IOObjectConformsTo(candidate, kIOUSBInterfaceClassName);
        if (!conforms)
        {
            IOObjectRelease(candidate);
            continue;
        }

        // Get bInterfaceNumber.
        int32_t actual_num;
        error = get_int32(candidate, CFSTR("bInterfaceNumber"), &actual_num);

        if (error == NULL && actual_num == interface_number)
        {
            // This is the right one.  Pass it to the caller.
            *interface_service = candidate;
            break;
        }

        IOObjectRelease(candidate);
    }

    if (error == NULL && *interface_service == MACH_PORT_NULL)
    {
        error = error_create("Could not find interface %d.", interface_number);
        error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
    }

    if (iterator != MACH_PORT_NULL) { IOObjectRelease(iterator); }
    return error;
}

libusbp_error * service_get_child_by_class(io_service_t service,
    const char * class_name, io_service_t * interface_service)
{
    assert(service != MACH_PORT_NULL);
    assert(interface_service != NULL);

    *interface_service = MACH_PORT_NULL;

    libusbp_error * error = NULL;

    io_iterator_t iterator = MACH_PORT_NULL;
    kern_return_t result = IORegistryEntryCreateIterator(
        service, kIOServicePlane, kIORegistryIterateRecursively, &iterator);

    if (result != KERN_SUCCESS)
    {
        error = error_create_mach(result, "Failed to get recursive iterator.");
    }

    // Loop through the devices to find the right one.
    while (error == NULL)
    {
        io_service_t candidate = IOIteratorNext(iterator);
        if (candidate == MACH_PORT_NULL) { break; }

        // Filter out candidates that are not the right class.
        bool conforms = (bool) IOObjectConformsTo(candidate, class_name);
        if (!conforms)
        {
            IOObjectRelease(candidate);
            continue;
        }

        // This is the right one.  Pass it to the caller.
        *interface_service = candidate;
        break;
    }

    if (error == NULL && *interface_service == MACH_PORT_NULL)
    {
        error = error_create("Could not find entry with class %s.", class_name);
        error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
    }

    if (iterator != MACH_PORT_NULL) { IOObjectRelease(iterator); }
    return error;
}

libusbp_error * service_to_interface(
    io_service_t service,
    CFUUIDRef pluginType,
    REFIID rid,
    void ** object,
    IOCFPlugInInterface *** plug_in)
{
    assert(service != MACH_PORT_NULL);
    assert(object != NULL);

    *object = NULL;

    int32_t score;

    libusbp_error * error = NULL;

    // Create the plug-in interface.
    IOCFPlugInInterface ** new_plug_in = NULL;
    kern_return_t kr = IOCreatePlugInInterfaceForService(service,
        pluginType, kIOCFPlugInInterfaceID,
        &new_plug_in, &score);

    if (kr != KERN_SUCCESS)
    {
        error = error_create_mach(kr, "Failed to create plug-in interface.");
    }

    // Create the device interface and pass it to the caller.
    if (error == NULL)
    {
        HRESULT hr = (*new_plug_in)->QueryInterface(new_plug_in, rid, object);
        if (hr)
        {
            error = error_create_hr(hr, "Failed to query interface.");
        }
    }

    // Also pass the plug-in interface to the caller if they want it.
    if (error == NULL && plug_in != NULL)
    {
        *plug_in = new_plug_in;
        new_plug_in = NULL;
    }

    // Clean up.
    if (new_plug_in != NULL)
    {
        (*new_plug_in)->Release(new_plug_in);
    }
    return error;
}


libusbp_error * get_id(io_registry_entry_t entry, uint64_t * id)
{
    assert(entry != MACH_PORT_NULL);
    assert(id != NULL);

    *id = 0;

    kern_return_t result = IORegistryEntryGetRegistryEntryID(entry, id);
    if (result != KERN_SUCCESS)
    {
        return error_create_mach(result, "Failed to get registry entry ID.");
    }
    return NULL;
}

// Returns NULL if the string is not present.
// The returned string should be freed with libusbp_string_free.
libusbp_error * get_string(io_registry_entry_t entry, CFStringRef name, char ** value)
{
    assert(entry != MACH_PORT_NULL);
    assert(name != NULL);
    assert(value != NULL);

    *value = NULL;

    CFTypeRef cf_value = IORegistryEntryCreateCFProperty(entry, name, kCFAllocatorDefault, 0);
    if (cf_value == NULL)
    {
        // The string probably does not exist, so just return.
        return NULL;
    }

    libusbp_error * error = NULL;

    if (CFGetTypeID(cf_value) != CFStringGetTypeID())
    {
        error = error_create("Property is not a string.");
    }

    char buffer[256];
    if (error == NULL)
    {
        bool success = CFStringGetCString(cf_value, buffer, sizeof(buffer),
            kCFStringEncodingASCII);
        if (!success)
        {
            error = error_create("Failed to convert property to C string.");
        }
    }

    if (error == NULL)
    {
        error = string_copy(buffer, value);
    }

    CFRelease(cf_value);
    return error;
}

libusbp_error * get_int32(io_registry_entry_t entry, CFStringRef name, int32_t * value)
{
    assert(entry != MACH_PORT_NULL);
    assert(name != NULL);
    assert(value != NULL);

    *value = 0;

    libusbp_error * error = NULL;

    CFTypeRef cf_value = IORegistryEntryCreateCFProperty(entry, name, kCFAllocatorDefault, 0);

    if (cf_value == NULL)
    {
        error = error_create("Failed to get int32 property from IORegistryEntry.");
    }

    if (error == NULL && CFGetTypeID(cf_value) != CFNumberGetTypeID())
    {
        error = error_create("Property is not a number.");
    }

    if (error == NULL)
    {
        bool success = CFNumberGetValue(cf_value, kCFNumberSInt32Type, value);
        if (!success)
        {
            error = error_create("Failed to convert property to C integer.");
        }
    }

    if (cf_value != NULL) { CFRelease(cf_value); }
    return error;
}

libusbp_error * get_uint16(io_registry_entry_t entry, CFStringRef name, uint16_t * value)
{
    assert(entry != MACH_PORT_NULL);
    assert(name != NULL);
    assert(value != NULL);

    libusbp_error * error = NULL;

    int32_t tmp;
    error = get_int32(entry, name, &tmp);

    if (error == NULL)
    {
        // There is an unchecked conversion of an int32_t to a uint16_t here but
        // we don't expect any data to be lost.
        *value = (uint16_t) tmp;
    }

    return error;
}

