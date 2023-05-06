// If we are compiling under MSVC, this is the file that will define GUID
// symbols we need such as GUID_DEVINTERFACE_USB_DEVICE.  This prevents
// an unresolved symbol error at link time.
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <initguid.h>
#endif

#include <libusbp_internal.h>

static libusbp_error * get_interface_non_composite(
    const char * device_instance_id,
    HDEVINFO * list,
    PSP_DEVINFO_DATA info)
{
    *list = INVALID_HANDLE_VALUE;

    // This is a non-composite device, so we just want to find a device
    // with the same device instance ID.
    libusbp_error * error = NULL;

    char id[MAX_DEVICE_ID_LEN + 1];

    HDEVINFO new_list = INVALID_HANDLE_VALUE;
    if (error == NULL)
    {
        new_list = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0,
            DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
        if (list == INVALID_HANDLE_VALUE)
        {
            error = error_create_winapi("Failed to list all USB devices.");
        }
    }

    if (error == NULL)
    {
        for(DWORD i = 0; ; i++)
        {
            SP_DEVINFO_DATA device_info_data;
            device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
            bool success = SetupDiEnumDeviceInfo(new_list, i, &device_info_data);
            if (!success)
            {
                if (GetLastError() == ERROR_NO_MORE_ITEMS)
                {
                    error = error_create("Failed to find device node.");
                }
                else
                {
                    error = error_create_winapi("Failed to enumerate device info.");
                }
                break;
            }

            success = SetupDiGetDeviceInstanceId(new_list, &device_info_data,
                id, sizeof(id), NULL);
            if (!success)
            {
                error = error_create_winapi("Failed to get device instance ID.");
                break;
            }

            if (strcmp(id, device_instance_id) == 0)
            {
                // We found the device; success.
                *list = new_list;
                new_list = INVALID_HANDLE_VALUE;
                *info = device_info_data;
                break;
            }
        }
    }

    if (new_list != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(new_list);
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to find non-composite device node.");
    }
    return error;
}

static libusbp_error * get_interface_composite(
    const char * device_instance_id,
    uint8_t interface_number,
    HDEVINFO * list,
    PSP_DEVINFO_DATA info)
{
    // This is a composite device, and we need to find a device
    // whose parent has the specified device_instance_id, and
    // whose own device instance ID has "MI_xx" where xx is the
    // hex representation of interface_number.

    *list = INVALID_HANDLE_VALUE;

    // Get the DEVINST for this device.
    CONFIGRET cr;
    DEVINST dev_inst;
    cr = CM_Locate_DevNode(&dev_inst, (char *)device_instance_id, CM_LOCATE_DEVNODE_NORMAL);
    if (cr != CR_SUCCESS)
    {
        libusbp_error * error = error_create_cr(cr,
            "Failed to get device node in order to find an interface.");

        // NOTE: if cr == CR_NO_SUCH_DEVNODE, that means
        // The device instance ID has a valid format, but either
        // the device it was referring to is unplugged or it was
        // never plugged into the computer in the first place.
        // This error has been seen when unplugging a USB device.
        return error;
    }

    unsigned int list_index = 0;
    HDEVINFO new_list = INVALID_HANDLE_VALUE;
    DWORD i = 0;

    // Iterate through various device lists until we find a device whose
    // parent device is ours and which controls the interface
    // specified by the caller.

    while (true)
    {
        if (new_list == INVALID_HANDLE_VALUE)
        {
            if (list_index == 0)
            {
                // Get a list of all the USB-related devices.
                // It includes native USB interfaces and usbser.sys ports but
                // not FTDI ports.
                new_list = SetupDiGetClassDevs(NULL,
                  "USB", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
            }
            else if (list_index == 1)
            {
                // Get a list of all the COM port devices.
                // This includes FTDI and usbser.sys ports.
                new_list = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT,
                  NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
            }
            else if (list_index == 2)
            {
                // Get a list of all modem devices.
                // Rationale: https://github.com/pyserial/pyserial/commit/7bb1dcc5aea16ca1c957690cb5276df33af1c286
                new_list = SetupDiGetClassDevs(&GUID_DEVINTERFACE_MODEM,
                  NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
            }
            else
            {
                // Could not find the child interface in any list.
                // This could be a temporary condition.
                libusbp_error * error = error_create("Could not find interface %d.",
                    interface_number);
                error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
                return error;
            }

            if (new_list == INVALID_HANDLE_VALUE)
            {
                return error_create_winapi(
                    "Failed to list devices to find an interface (%u).",
                    list_index);
            }
            i = 0;
        }

        SP_DEVINFO_DATA device_info_data;
        device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
        bool success = SetupDiEnumDeviceInfo(new_list, i, &device_info_data);
        if (!success)
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
            {
                // This list is done.  Try the next list.
                SetupDiDestroyDeviceInfoList(new_list);
                new_list = INVALID_HANDLE_VALUE;
                list_index++;
                continue;
            }
            else
            {
                libusbp_error * error = error_create_winapi(
                    "Failed to get device info to find an interface.");
                SetupDiDestroyDeviceInfoList(new_list);
                return error;
            }
        }

        DEVINST parent_dev_inst;
        cr = CM_Get_Parent(&parent_dev_inst, device_info_data.DevInst, 0);
        if (cr != CR_SUCCESS)
        {
            SetupDiDestroyDeviceInfoList(new_list);
            return error_create_cr(cr, "Failed to get parent of an interface.");
        }

        if (parent_dev_inst != dev_inst)
        {
            // This device is not a child of our device.
            i++;
            continue;
        }

        // Get the device instance ID.
        char device_id[MAX_DEVICE_ID_LEN + 1];
        cr = CM_Get_Device_ID(device_info_data.DevInst, device_id, sizeof(device_id), 0);
        if (cr != CR_SUCCESS)
        {
            libusbp_error * error = error_create_cr(cr,
                "Failed to get device instance ID while finding an interface.");
            SetupDiDestroyDeviceInfoList(new_list);
            return error;
        }

        unsigned int actual_interface_number;
        int result = sscanf(device_id, "USB\\VID_%*4x&PID_%*4x&MI_%2x\\",
            &actual_interface_number);
        if (result != 1)
        {
          result = sscanf(device_id, "FTDIBUS\\%*[^\\]\\%x",
            &actual_interface_number);
          if (result != 1)
          {
            // Could not figure out the interface number.
            i++;
            continue;
          }
        }
        if (actual_interface_number != interface_number)
        {
            // This is not the right interface.
            i++;
            continue;
        }

        // Found the interface.
        *list = new_list;
        *info = device_info_data;
        return NULL;
    }
}

libusbp_error * get_interface(
    const char * device_instance_id,
    uint8_t interface_number,
    bool composite,
    HDEVINFO * list,
    PSP_DEVINFO_DATA info)
{
    assert(device_instance_id != NULL);
    assert(list != NULL);
    assert(info != NULL);

    if (composite)
    {
        return get_interface_composite(device_instance_id, interface_number,
            list, info);
    }
    else
    {
        return get_interface_non_composite(device_instance_id, list, info);
    }
}

libusbp_error * get_filename_from_devinst_and_guid(
    DEVINST devinst,
    const GUID * guid,
    char ** filename
    )
{
    assert(guid != NULL);
    assert(filename != NULL);

    BOOL success;

    // Make a list of devices that have the specified device interface GUID.
    HDEVINFO list = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (list == INVALID_HANDLE_VALUE)
    {
        return error_create_winapi(
            "Failed to create a list of devices to find a device filename.");
    }

    // Iterate through the list looking for one that matches the given DEVINST.
    SP_DEVINFO_DATA device_info_data;
    for(DWORD index = 0; ; index++)
    {
        device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
        success = SetupDiEnumDeviceInfo(list, index, &device_info_data);
        if (!success)
        {
            if (GetLastError() == ERROR_NO_MORE_ITEMS)
            {
                // We reached the end of the list.
                SetupDiDestroyDeviceInfoList(list);
                libusbp_error * error = error_create(
                    "Could not find matching device in order to get its filename.");

                // This is one of the errors we see when plugging in a device,
                // so it could indicate that the devce is just not ready and
                // Windows is still setting it up.
                error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
                return error;
            }

            libusbp_error * error = error_create_winapi(
                "Failed to enumerate list item to find matching device.");
            SetupDiDestroyDeviceInfoList(list);
            return error;
        }

        if (device_info_data.DevInst == devinst)
        {
            // We found the matching device, so break.
            break;
        }
    }

    // Get the DeviceInterfaceData struct.
    SP_DEVICE_INTERFACE_DATA device_interface_data;
    device_interface_data.cbSize = sizeof(device_interface_data);
    success = SetupDiEnumDeviceInterfaces(list, &device_info_data, guid, 0, &device_interface_data);
    if (!success)
    {
        libusbp_error * error = error_create_winapi("Failed to get device interface data.");
        SetupDiDestroyDeviceInfoList(list);
        return error;
    }

    // Get the DeviceInterfaceDetailData struct size.
    DWORD size;
    success = SetupDiGetDeviceInterfaceDetail(list, &device_interface_data, NULL, 0, &size, NULL);
    if (!success && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        libusbp_error * error = error_create_winapi("Failed to get the size of the device interface details.");
        SetupDiDestroyDeviceInfoList(list);
        return error;
    }

    // Get the DeviceInterfaceDetailData struct data
    SP_DEVICE_INTERFACE_DETAIL_DATA_A * device_interface_detail_data = malloc(size);
    if (device_interface_detail_data == NULL)
    {
        return &error_no_memory;
    }
    device_interface_detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
    success = SetupDiGetDeviceInterfaceDetail(list, &device_interface_data,
        device_interface_detail_data, size, NULL, NULL);
    if (!success)
    {
        libusbp_error * error = error_create_winapi("Failed to get the device interface details.");
        free(device_interface_detail_data);
        SetupDiDestroyDeviceInfoList(list);
        return error;
    }

    char * new_string = strdup(device_interface_detail_data->DevicePath);
    if (new_string == NULL)
    {
        free(device_interface_detail_data);
        SetupDiDestroyDeviceInfoList(list);
        return &error_no_memory;
    }

    *filename = new_string;

    free(device_interface_detail_data);
    SetupDiDestroyDeviceInfoList(list);
    return NULL;
}
