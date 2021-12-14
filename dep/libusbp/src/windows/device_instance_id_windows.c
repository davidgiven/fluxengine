#include <libusbp_internal.h>

libusbp_error * create_id_string(HDEVINFO list, PSP_DEVINFO_DATA info, char ** id)
{
    assert(list != INVALID_HANDLE_VALUE);
    assert(info != NULL);
    assert(id != NULL);

    DWORD size = MAX_DEVICE_ID_LEN + 1;
    char * new_id = malloc(size);
    if (new_id == NULL)
    {
        return &error_no_memory;
    }

    bool success = SetupDiGetDeviceInstanceId(list, info, new_id, size, NULL);
    if (!success)
    {
        free(new_id);
        return error_create_winapi("Error getting device instance ID.");
    }

    *id = new_id;
    return NULL;
}
