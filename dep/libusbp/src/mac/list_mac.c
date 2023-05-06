#include <libusbp_internal.h>

static void try_create_device(io_service_t service, libusbp_device ** device)
{
    libusbp_error * error = create_device(service, device);
    if (error != NULL)
    {
        assert(*device == NULL);

        // Something went wrong.  To make the library more robust and usable, we
        // ignore this error and continue.
        #ifdef LIBUSBP_LOG
        fprintf(stderr, "Problem creating device: %s\n",
            libusbp_error_get_message(error));
        #endif

        libusbp_error_free(error);
    }
}

libusbp_error * libusbp_list_connected_devices(
    libusbp_device *** device_list,
    size_t * device_count)
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

    // Create a dictionary that says "IOProviderClass" => "IOUSBDevice"
    // This dictionary is CFReleased by IOServiceGetMatchingServices.
    CFMutableDictionaryRef dict = NULL;
    if (error == NULL)
    {
        dict = IOServiceMatching("IOUSBHostDevice");
        if (dict == NULL)
        {
            error = error_create("IOServiceMatching returned null.");
        }
    }

    // Create an iterator for all the connected USB devices.
    io_iterator_t iterator = MACH_PORT_NULL;
    if (error == NULL)
    {
        // IOServiceGetMatchingServices consumes one reference to dict,
        // so we don't have to CFRelease it.
        kern_return_t result = IOServiceGetMatchingServices(
            kIOMasterPortDefault, dict, &iterator);
        dict = NULL;
        if (result != KERN_SUCCESS)
        {
            error = error_create_mach(result, "Failed to get matching services.");
        }
    }

    // Allocate a new list.
    libusbp_device ** new_list = NULL;
    size_t count = 0;
    if (error == NULL)
    {
        error = device_list_create(&new_list);
    }

    // Loop through the devices and add them to our list.
    while(error == NULL)
    {
        io_service_t service = IOIteratorNext(iterator);
        if (service == MACH_PORT_NULL) { break; }

        libusbp_device * device = NULL;
        try_create_device(service, &device);
        if (device != NULL)
        {
            error = device_list_append(&new_list, &count, device);
        }

        IOObjectRelease(service);
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

    // Clean up.
    assert(dict == NULL);
    if (dict != NULL) { CFRelease(dict); }  // makes the code less brittle
    if (iterator != MACH_PORT_NULL) { IOObjectRelease(iterator); }
    free_devices_and_list(new_list);
    return error;
}
