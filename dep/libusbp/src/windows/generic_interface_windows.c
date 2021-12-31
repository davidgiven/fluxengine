#include <libusbp_internal.h>

struct libusbp_generic_interface
{
    uint8_t interface_number;
    char * device_instance_id;
    char * filename;
};

// Returns NULL via the driver_name parameter if no driver is installed.
// Otherwise returns the driver name.
static libusbp_error * get_driver_name(
    HDEVINFO list, PSP_DEVINFO_DATA info, char ** driver_name)
{
    assert(list != INVALID_HANDLE_VALUE);
    assert(info != NULL);
    assert(driver_name != NULL);

    *driver_name = NULL;

    // Get the "Service" key from the registry, which will say what
    // Windows service is assigned to this device.
    CHAR service[64];
    DWORD data_type;
    bool success = SetupDiGetDeviceRegistryProperty(list, info, SPDRP_SERVICE,
        &data_type, (BYTE *)&service, sizeof(service), NULL);
    if (!success)
    {
        if (GetLastError() == ERROR_INVALID_DATA)
        {
            // The registry key isn't present, so the device does not
            // have have a driver.
            return NULL;
        }
        else
        {
            return error_create_winapi("Failed to get the service name.");
        }
    }
    if (data_type != REG_SZ)
    {
        return error_create("Service name is the wrong data type: %ld.", data_type);
    }

    return string_copy(service, driver_name);
}

// Checks to see if a suitable driver for a USB generic interface is installed
// by reading the Service registry key.
static libusbp_error * check_driver_installation(
    HDEVINFO list, PSP_DEVINFO_DATA info,
    bool * good_driver_installed, bool * bad_driver_installed,
    char ** driver_name)
{
    assert(list != INVALID_HANDLE_VALUE);
    assert(info != NULL);
    assert(good_driver_installed != NULL);
    assert(bad_driver_installed != NULL);
    assert(driver_name != NULL);

    *good_driver_installed = false;
    *bad_driver_installed = false;
    *driver_name = NULL;

    libusbp_error * error;

    error = get_driver_name(list, info, driver_name);
    if (error != NULL)
    {
        return error;
    }

    if (*driver_name == NULL)
    {
        // No driver installed.
        return NULL;
    }

    // We really don't want this comparison to be affected by the user's locale,
    // since that will probably just mess things up.
    int result = CompareString(LOCALE_INVARIANT, NORM_IGNORECASE,
        *driver_name, -1, "winusb", -1);
    if (result == CSTR_EQUAL)
    {
        *good_driver_installed = true;
        return NULL;
    }

    *bad_driver_installed = true;
    return NULL;
}

static libusbp_error * get_first_device_interface_guid(HDEVINFO list,
    PSP_DEVINFO_DATA info, GUID * guid)
{
    assert(list != INVALID_HANDLE_VALUE);
    assert(info != NULL);
    assert(guid != NULL);

    HANDLE key = SetupDiOpenDevRegKey(list, info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if (key == INVALID_HANDLE_VALUE)
    {
        return error_create_winapi(
            "Failed to get device registry key in order to find its device interface GUIDs.");
    }

    // Get the size of the DeviceInterfaceGUIDs key.
    // (Partial reads are not allowed.)
    DWORD size;
    LONG reg_result;
    reg_result = RegQueryValueExW(key, L"DeviceInterfaceGUIDs", NULL, NULL, NULL, &size);
    if (reg_result != ERROR_SUCCESS)
    {
        if (reg_result == ERROR_FILE_NOT_FOUND)
        {
            RegCloseKey(key);
            return error_create("DeviceInterfaceGUIDs key does not exist.");
        }
        else
        {
            RegCloseKey(key);
            SetLastError(reg_result);
            return error_create_winapi("Failed to get DeviceInterfaceGUIDs key size.");
        }
    }

    WCHAR * guids = malloc(size);
    if (guids == NULL)
    {
        return &error_no_memory;
    }

    DWORD reg_type;
    reg_result = RegQueryValueExW(key, L"DeviceInterfaceGUIDs",
        NULL, &reg_type, (BYTE *)guids, &size);
    RegCloseKey(key);
    if (reg_result)
    {
        free(guids);
        SetLastError(reg_result);
        return error_create_winapi("Failed to get DeviceInterfaceGUIDs key.");
    }

    if (reg_type != REG_MULTI_SZ)
    {
        free(guids);
        return error_create(
            "Expected DeviceInterfaceGUIDs key to be a REG_MULTI_SZ (0x%x), got 0x%lx.",
            REG_MULTI_SZ, reg_type);
    }

    HRESULT hr = IIDFromString(guids, guid);
    free(guids);
    if (FAILED(hr))
    {
        return error_create_hr(hr, "Failed to parse device interface GUID.");
    }

    return NULL;
}

static libusbp_error * generic_interface_initialize(libusbp_generic_interface * gi,
    const libusbp_device * device, uint8_t interface_number, bool composite)
{
    assert(gi != NULL);
    assert(device != NULL);

    // First, initialize everything so that the struct can safely be
    // freed if something goes wrong.
    gi->interface_number = interface_number;
    gi->device_instance_id = NULL;
    gi->filename = NULL;

    libusbp_error * error = NULL;

    // Get the device instance ID of the overall USB device.
    char * usb_device_id;
    if (error == NULL)
    {
        error = libusbp_device_get_os_id(device, &usb_device_id);
    }

    // Access the generic interface device node with SetupAPI.
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
        error = create_id_string(list, &device_info_data, &gi->device_instance_id);
    }

    // Check the driver situation.
    char * driver_name = NULL;
    if (error == NULL)
    {
        bool good_driver_installed = false;
        bool bad_driver_installed = false;
        error = check_driver_installation(list, &device_info_data,
            &good_driver_installed, &bad_driver_installed, &driver_name);
        if (error == NULL && bad_driver_installed)
        {
            error = error_create("Device is attached to an incorrect driver: %s.", driver_name);
        }
        if (error == NULL && !good_driver_installed)
        {
            error = error_create("Device is not using any driver.");
            error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
        }
    }

    // Get the first device interface GUID.
    GUID guid;
    if (error == NULL)
    {
      error = get_first_device_interface_guid(list, &device_info_data, &guid);
    }

    // Use that GUID to get an actual filename that we can later open
    // with CreateFile to access the device.
    if (error == NULL)
    {
        error = get_filename_from_devinst_and_guid(device_info_data.DevInst,
            &guid, &gi->filename);
    }

    // Clean up.

    if (list != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(list);
    }
    libusbp_string_free(driver_name);
    libusbp_string_free(usb_device_id);
    if (error != NULL)
    {
        error = error_add(error, "Failed to initialize generic interface.");
    }
    return error;
}

libusbp_error * libusbp_generic_interface_create(
    const libusbp_device * device,
    uint8_t interface_number,
    bool composite,
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

    libusbp_generic_interface * new_gi = malloc(sizeof(libusbp_generic_interface));
    if (new_gi == NULL)
    {
        return &error_no_memory;
    }

    libusbp_error * error = generic_interface_initialize(new_gi, device,
        interface_number, composite);
    if (error)
    {
        libusbp_generic_interface_free(new_gi);
        return error;
    }
    *gi = new_gi;

    return NULL;
}

libusbp_error * libusbp_generic_interface_copy(
    const libusbp_generic_interface * source,
    libusbp_generic_interface ** dest
)
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

    assert(source->device_instance_id);
    assert(source->filename);

    libusbp_generic_interface * new_gi = calloc(1, sizeof(libusbp_generic_interface));
    char * id = strdup(source->device_instance_id);
    char * filename = strdup(source->filename);
    if (new_gi == NULL || id == NULL || filename == NULL)
    {
        free(new_gi);
        free(id);
        free(filename);
        return &error_no_memory;
    }

    new_gi->interface_number = source->interface_number;
    new_gi->device_instance_id = id;
    new_gi->filename = filename;
    *dest = new_gi;

    return NULL;
}

void libusbp_generic_interface_free(libusbp_generic_interface * gi)
{
    if (gi != NULL)
    {
        free(gi->device_instance_id);
        free(gi->filename);
        free(gi);
    }
}

libusbp_error * libusbp_generic_interface_get_os_id(
    const libusbp_generic_interface * gi,
    char ** id)
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
    return string_copy(gi->device_instance_id, id);
}

libusbp_error * libusbp_generic_interface_get_os_filename(
    const libusbp_generic_interface * gi,
    char ** filename)
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
    return string_copy(gi->filename, filename);
}
