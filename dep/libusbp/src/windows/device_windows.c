#include <libusbp_internal.h>

struct libusbp_device
{
    // A string like: USB\VID_xxxx&PID_xxxx\idthing
    char * device_instance_id;
    uint16_t product_id;
    uint16_t vendor_id;
    uint16_t revision;
};

static libusbp_error * device_allocate(libusbp_device ** device)
{
    assert(device != NULL);
    *device = calloc(1, sizeof(libusbp_device));
    if (*device == NULL)
    {
        return &error_no_memory;
    }
    return NULL;
}

// Gets the hardware IDs of the device, in ASCII REG_MULTI_SZ format.  If there
// is no error, the returned IDs must be freed with libusbp_string_free.
// This function converts the IDs to uppercase before returning them.
static libusbp_error * device_get_hardware_ids(
    HDEVINFO list, PSP_DEVINFO_DATA info, char ** ids)
{
    assert(list != INVALID_HANDLE_VALUE);
    assert(info != NULL);
    assert(ids != NULL);

    *ids = NULL;

    libusbp_error * error = NULL;

    // Get the size of the hardware IDs.
    DWORD data_type = 0;
    DWORD size = 0;
    if (error == NULL)
    {
        BOOL success = SetupDiGetDeviceRegistryProperty(list, info,
            SPDRP_HARDWAREID, &data_type, NULL, 0, &size);
        if (!success && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            error = error_create_winapi("Failed to get size of hardware IDs.");
        }
    }

    // Allocate memory.
    char * new_ids = NULL;
    if (error == NULL)
    {
        new_ids = malloc(size);
        if (new_ids == NULL)
        {
            error = &error_no_memory;
        }
    }

    // Get the actual hardware IDs.
    if (error == NULL)
    {
        BOOL success = SetupDiGetDeviceRegistryProperty(list, info,
            SPDRP_HARDWAREID, &data_type, (unsigned char *)new_ids, size, NULL);
        if (!success)
        {
            error = error_create_winapi("Failed to get hardware IDs.");
        }
    }

    // Check the data type.
    if (error == NULL && data_type != REG_MULTI_SZ)
    {
        error = error_create("Hardware IDs are wrong data type: %ld.", data_type);
    }
    if (error == NULL && (size < 2 || new_ids[size - 2] != 0 || new_ids[size - 1] != 0))
    {
        error = error_create("Hardware IDs are empty or not terminated correctly.");
    }

    // Capitalize the hardware IDs because some drivers create USB IDs with the
    // wrong capitalization (i.e. "Vid" instead of "VID").
    if (error == NULL)
    {
        for (DWORD i = 0; i < size; i++)
        {
            if (new_ids[i] >= 'a' && new_ids[i] <= 'z')
            {
                new_ids[i] -= 'a' - 'A';
            }
        }
    }

    // Pass the IDs to the caller.
    if (error == NULL)
    {
        *ids = new_ids;
        new_ids = NULL;
    }

    free(new_ids);
    return error;
}

// This function extracts the product ID, vendor ID, and revision code
// from the hardware IDs.
// The "ids" parmaeter must be a pointer to a REG_MULTI_SZ data structure.
// (null-terminated ASCII strings, ending with an empty string).
static libusbp_error * device_take_info_from_hardware_ids(
    libusbp_device * device, const char * ids)
{
    assert(ids != NULL);
    assert(device != NULL);

    device->vendor_id = 0;
    device->product_id = 0;
    device->revision = 0;

    for(; *ids; ids += strlen(ids) + 1)
    {
        uint16_t vendor_id, product_id, revision;
        int result = sscanf(ids, "USB\\VID_%4hx&PID_%4hx&REV_%4hx",
            &vendor_id, &product_id, &revision);
        if (result == 3)
        {
            device->vendor_id = vendor_id;
            device->product_id = product_id;
            device->revision = revision;
            return NULL;
        }
    }

    return error_create("Device has no hardware ID with the correct format.");
}

libusbp_error * create_device(HDEVINFO list, PSP_DEVINFO_DATA info, libusbp_device ** device)
{
    assert(list != INVALID_HANDLE_VALUE);
    assert(info != NULL);
    assert(info->cbSize == sizeof(SP_DEVINFO_DATA));
    assert(device != NULL);

    *device = NULL;

    libusbp_error * error = NULL;

    // Allocate memory for the device itself.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        error = device_allocate(&new_device);
    }

    // Get the device instance ID.
    if (error == NULL)
    {
        error = create_id_string(list, info, &new_device->device_instance_id);
    }

    // Get the vendor ID, product ID, and revision from the hardware IDs.
    char * new_ids = NULL;
    if (error == NULL)
    {
        error = device_get_hardware_ids(list, info, &new_ids);
    }
    if (error == NULL)
    {
        error = device_take_info_from_hardware_ids(new_device, new_ids);
    }

    // Return the device to the caller.
    if (error == NULL)
    {
        *device = new_device;
        new_device = NULL;
    }

    libusbp_string_free(new_ids);
    libusbp_device_free(new_device);
    return error;
}

libusbp_error * libusbp_device_copy(const libusbp_device * source, libusbp_device ** dest)
{
    if (dest == NULL)
    {
        return error_create("Device output pointer is null.");
    }

    *dest = NULL;

    if (source == NULL)
    {
        return NULL;
    }

    libusbp_error * error = NULL;

    // Allocate memory for the device itself.
    libusbp_device * new_device = NULL;
    if (error == NULL)
    {
        error = device_allocate(&new_device);
    }

    // Copy the device instance ID.
    if (error == NULL)
    {
        assert(source->device_instance_id != NULL);
        size_t size = strlen(source->device_instance_id) + 1;
        char * new_id = malloc(size);
        if (new_id == NULL)
        {
            error = &error_no_memory;
        }
        else
        {
            memcpy(new_id, source->device_instance_id, size);
            new_device->device_instance_id = new_id;
        }
    }

    if (error == NULL)
    {
        new_device->vendor_id = source->vendor_id;
        new_device->product_id = source->product_id;
        new_device->revision = source->revision;
    }

    // Pass the device to the caller.
    if (error == NULL)
    {
        *dest = new_device;
        new_device = NULL;
    }

    libusbp_device_free(new_device);
    return error;
}

// Note: Some pointers in the device might be null when we are
// freeing, because this can be called from the library to clean up a
// failed device creation.
void libusbp_device_free(libusbp_device * device)
{
    if (device != NULL)
    {
        free(device->device_instance_id);
        free(device);
    }
}

libusbp_error * libusbp_device_get_vendor_id(
    const libusbp_device * device, uint16_t * vendor_id)
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
    const libusbp_device * device, uint16_t * product_id)
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
    const libusbp_device * device, uint16_t * revision)
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
    const libusbp_device * device, char ** serial_number)
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

    libusbp_error * error = NULL;

    if (error == NULL && strlen(device->device_instance_id) < 22)
    {
        error = error_create("Device instance ID is too short.");
    }

    if (error == NULL)
    {
        error = string_copy(device->device_instance_id + 22, serial_number);
    }

    if (error != NULL)
    {
        error = error_add(error, "Failed to get serial number.");
    }
    return error;
}

libusbp_error * libusbp_device_get_os_id(
    const libusbp_device * device, char ** id)
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

    return string_copy(device->device_instance_id, id);
}

