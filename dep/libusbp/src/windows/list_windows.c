#include <libusbp_internal.h>

#if SIZE_MAX < DWORD_MAX
#error The code in get_list_length assumes a size_t can hold any DWORD.
#endif

static void try_create_device(HDEVINFO list, PSP_DEVINFO_DATA info,
    libusbp_device ** device)
{
    libusbp_error * error = create_device(list, info, device);
    if (error != NULL)
    {
        assert(*device == NULL);

        // Something went wrong.  For example, one of the devices might have
        // lacked a hardware ID with the proper format.  To make the library
        // more robust and usable, we ignore this error and continue.
        #ifdef LIBUSBP_LOG
        fprintf(stderr, "Problem creating device: %s\n",
            libusbp_error_get_message(error));
        #endif

        libusbp_error_free(error);
    }
}

libusbp_error * libusbp_list_connected_devices(libusbp_device *** device_list,
    size_t * device_count)
{
    // Return 0 for the device count by default, just to be safe.
    if (device_count != NULL)
    {
        *device_count = 0;
    }

    if (device_list == NULL)
    {
        return error_create("Device list output pointer is null.");
    }

    libusbp_error * error = NULL;

    // Get a list of all USB devices from Windows.
    HDEVINFO handle = INVALID_HANDLE_VALUE;
    if (error == NULL)
    {
        handle = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0,
            DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
        if (handle == INVALID_HANDLE_VALUE)
        {
            error = error_create_winapi("Failed to list all USB devices.");
        }
    }

    // Start a new device list and keep track of how many devices are in it.
    libusbp_device ** new_list = NULL;
    size_t count = 0;
    if (error == NULL)
    {
        error = device_list_create(&new_list);
    }

    // Each iteration of this loop attempts to set up a new device.
    DWORD index = 0;
    while(error == NULL)
    {
        // See if we have reached the end of the list.
        SP_DEVINFO_DATA info;
        info.cbSize = sizeof(info);
        BOOL success = SetupDiEnumDeviceInfo(handle, index, &info);
        if (!success)
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
            {
                // We have reached the end of the list.
                break;
            }

            // An unexpected error happened.
            error = error_create_winapi("Failed to test for the end of the USB device list.");
            break;
        }

        libusbp_device * device = NULL;
        try_create_device(handle, &info, &device);
        if (device != NULL)
        {
            error = device_list_append(&new_list, &count, device);
        }
        index++;
    }

    // Give the list and count to the caller.
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
    if (handle != INVALID_HANDLE_VALUE) { SetupDiDestroyDeviceInfoList(handle); }
    if (new_list != NULL) { free_devices_and_list(new_list); }
    return error;
}
