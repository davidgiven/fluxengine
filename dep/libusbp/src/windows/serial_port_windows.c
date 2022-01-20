#include <libusbp_internal.h>

struct libusbp_serial_port
{
    char * device_instance_id;
    char * port_name;  // e.g. "COM4"
};

libusbp_error * libusbp_serial_port_create(
    const libusbp_device * device,
    uint8_t interface_number,
    bool composite,
    libusbp_serial_port ** port)
{
    if (port == NULL)
    {
        return error_create("Serial port output pointer is null.");
    }

    *port = NULL;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    libusbp_error * error = NULL;

    libusbp_serial_port * new_sp = NULL;
    if (error == NULL)
    {
        new_sp = calloc(1, sizeof(libusbp_serial_port));
        if (new_sp == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Get the device instance ID of the overall USB device.
    char * usb_device_id = NULL;
    if (error == NULL)
    {
        error = libusbp_device_get_os_id(device, &usb_device_id);
    }

    // Access the serial port interface device node with SetupAPI.
    HDEVINFO list = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA device_info_data;
    if (error == NULL)
    {
        error = get_interface(usb_device_id, interface_number,
            composite, &list, &device_info_data);
    }

    // Record the device instance ID.
    if (error == NULL)
    {
        error = create_id_string(list, &device_info_data, &new_sp->device_instance_id);
    }

    // Open the registry key for device-specific configuration information.
    HANDLE keyDev = INVALID_HANDLE_VALUE;
    if (error == NULL)
    {
        keyDev = SetupDiOpenDevRegKey(list, &device_info_data,
            DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (keyDev == INVALID_HANDLE_VALUE)
        {
            error = error_create_winapi(
                "Failed to get device registry key in order to find its COM port name.");
        }
    }

    // Get the port name from the registry (e.g. "COM8").
    char portName[128];
    if (error == NULL)
    {
        DWORD size = sizeof(portName);
        DWORD reg_type;
        LONG reg_result = RegQueryValueEx(keyDev, "PortName",
            NULL, &reg_type, (BYTE *)portName, &size);
        if (reg_result != ERROR_SUCCESS)
        {
            if (reg_result == ERROR_FILE_NOT_FOUND)
            {
                error = error_create("The PortName key was not found.");
            }
            else
            {
                SetLastError(reg_result);
                error = error_create_winapi("Failed to get PortName key.");
            }
        }
        else if (reg_type != REG_SZ)
        {
            error = error_create(
                "Expected PortName key to be a REG_SZ (0x%x), got 0x%lx.",
                REG_SZ, reg_type);
        }
    }

    // Copy the port name into the serial port object.
    if (error == NULL)
    {
        error = string_copy(portName, &new_sp->port_name);
    }

    // Give the new serial port to the caller.
    if (error == NULL)
    {
        *port = new_sp;
        new_sp = NULL;
    }

    // Clean up.
    if (keyDev != INVALID_HANDLE_VALUE)
    {
        RegCloseKey(keyDev);
    }
    if (list != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(list);
    }
    libusbp_string_free(usb_device_id);
    libusbp_serial_port_free(new_sp);

    return error;
}

void libusbp_serial_port_free(libusbp_serial_port * port)
{
    if (port == NULL) { return; }
    libusbp_string_free(port->device_instance_id);
    libusbp_string_free(port->port_name);
    free(port);
}

libusbp_error * libusbp_serial_port_copy(const libusbp_serial_port * source,
    libusbp_serial_port ** dest)
{
    if (dest == NULL)
    {
        return error_create("Serial port output pointer is null.");
    }

    *dest = NULL;

    if (source == NULL)
    {
        return NULL;
    }

    assert(source->device_instance_id);
    assert(source->port_name);

    libusbp_error * error = NULL;

    libusbp_serial_port * new_sp = NULL;
    if (error == NULL)
    {
        new_sp = calloc(1, sizeof(libusbp_serial_port));
        if (new_sp == NULL)
        {
            error = &error_no_memory;
        }
    }

    if (error == NULL)
    {
        error = string_copy(source->device_instance_id, &new_sp->device_instance_id);
    }

    if (error == NULL)
    {
        error = string_copy(source->port_name, &new_sp->port_name);
    }

    if (error == NULL)
    {
        *dest = new_sp;
        new_sp = NULL;
    }

    libusbp_serial_port_free(new_sp);

    return error;
}

libusbp_error * libusbp_serial_port_get_name(
    const libusbp_serial_port * port,
    char ** name)
{
    if (name == NULL)
    {
        return error_create("String output pointer is null.");
    }

    *name = NULL;

    if (port == NULL)
    {
        return error_create("Serial port is null.");
    }

    return string_copy(port->port_name, name);
}

