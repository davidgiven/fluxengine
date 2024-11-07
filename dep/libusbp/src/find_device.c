#include <libusbp_internal.h>

static libusbp_error * check_device_vid_pid(const libusbp_device * device,
    uint16_t vendor_id, uint16_t product_id, bool * matches)
{
    assert(matches != NULL);

    *matches = false;

    libusbp_error * error;

    uint16_t device_vid;
    error = libusbp_device_get_vendor_id(device, &device_vid);
    if (error != NULL) { return error; }
    if (device_vid != vendor_id) { return NULL; }

    uint16_t device_pid;
    error = libusbp_device_get_product_id(device, &device_pid);
    if (error != NULL) { return error; }
    if (device_pid != product_id) { return NULL; }

    *matches = true;
    return NULL;
}

libusbp_error * libusbp_find_device_with_vid_pid(
    uint16_t vendor_id, uint16_t product_id, libusbp_device ** device)
{
    if (device == NULL)
    {
        return error_create("Device output pointer is null.");
    }

    *device = NULL;

    libusbp_error * error = NULL;

    libusbp_device ** new_list = NULL;
    size_t size = 0;
    if (error == NULL)
    {
        error = libusbp_list_connected_devices(&new_list, &size);
    }

    assert(error != NULL || new_list != NULL);

    for(size_t i = 0; error == NULL && i < size; i++)
    {
        // Each iteration of this loop checks one candidate device
        // and either passes it to the caller or frees it.

        libusbp_device * candidate = new_list[i];

        if (*device == NULL)
        {
            bool matches;
            error = check_device_vid_pid(candidate, vendor_id, product_id, &matches);
            if (error != NULL) { break; }

            if (matches)
            {
                // Return the device to the caller.
                *device = candidate;
                candidate = NULL;
            }
        }

        libusbp_device_free(candidate);
    }

    libusbp_list_free(new_list);
    return error;
}
