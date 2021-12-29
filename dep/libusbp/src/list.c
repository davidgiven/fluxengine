/** This file provides libusbp_list_free as well as internal functions that help
 * construct lists of devices.
 *
 * At all times, a list maintained by these functions will be NULL terminated.
 */

#include <libusbp_internal.h>

libusbp_error * device_list_create(libusbp_device *** device_list)
{
    assert(device_list != NULL);

    *device_list = NULL;

    libusbp_device ** new_list = malloc(sizeof(libusbp_device *));
    if (new_list == NULL)
    {
        return &error_no_memory;
    }

    new_list[0] = NULL;

    *device_list = new_list;
    return NULL;
}

libusbp_error * device_list_append(libusbp_device *** device_list,
    size_t * count, libusbp_device * device)
{
    assert(device_list != NULL);
    assert(count != NULL);
    assert(device != NULL);

    size_t new_count = *count + 1;
    libusbp_device ** expanded_list = realloc(*device_list,
        (new_count + 1) * sizeof(libusbp_device *));

    if (expanded_list == NULL)
    {
        // Expanding the list failed, so we return an error and leave the
        // list in its original state.
        return &error_no_memory;
    }

    expanded_list[new_count - 1] = device;
    expanded_list[new_count] = NULL;

    *count = new_count;
    *device_list = expanded_list;
    return NULL;
}

void free_devices_and_list(libusbp_device ** device_list)
{
    if (device_list != NULL)
    {
        libusbp_device ** device = device_list;
        while(*device != NULL)
        {
            libusbp_device_free(*device);
            device++;
        }
        libusbp_list_free(device_list);
    }
}

void libusbp_list_free(libusbp_device ** device_list)
{
    if (device_list == NULL) { return; }

    free(device_list);
}
