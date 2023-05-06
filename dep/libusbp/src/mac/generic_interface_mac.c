#include <libusbp_internal.h>

/* Composite devices on all OSes and non-composite vendor-defined devices on
 * Windows and Linux will automatically be set to configuration 1, which means
 * we can find device nodes for the specific interface we are interested in
 * using in libusbp_generic_interface_create, and return error code
 * LIBUSBP_ERROR_NOT_READY if that node is not found.  However, non-composite
 * devices on Mac OS X will not automatically get configured unless someone
 * tells them what configuration to use.
 *
 * For composite devices on Mac OS X:
 *   libusbp_generic_interface_create() finds the correct device node.
 *   libusbp_generic_handle_open() simply opens it.
 *
 * For non-composite devices on Mac OS X:
 *   libusbp_generic_interface_create() just records information from the user.
 *   libusbp_generic_handle_open() ensures the device is set to configuration 1,
 *     then finds the correct interface and opens it.
 */

struct libusbp_generic_interface
{
    uint64_t device_id;
    uint8_t interface_number;
    bool has_interface_id;
    uint64_t interface_id;
};

libusbp_error * generic_interface_allocate(libusbp_generic_interface ** gi)
{
    *gi = calloc(1, sizeof(libusbp_generic_interface));
    if (*gi == NULL)
    {
        return &error_no_memory;
    }
    return NULL;
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

    libusbp_error * error = NULL;

    libusbp_generic_interface * new_gi = NULL;
    if (error == NULL)
    {
        error = generic_interface_allocate(&new_gi);
    }

    // Record the I/O registry ID for the device.
    if (error == NULL)
    {
        new_gi->device_id = device_get_id(device);
    }

    // Record the interface number.
    if (error == NULL)
    {
        new_gi->interface_number = interface_number;
    }

    // If this is a composite device, get the id for the specific interface we
    // are interested in.  If it is non-composite, we wait until later becuase
    // the device might be unconfigured and its interface registry entries might
    // not even exist yet.
    if (error == NULL && composite)
    {
        // Get an io_service_t for the physical device.
        io_service_t device_service = MACH_PORT_NULL;
        if (error == NULL)
        {
            error = service_get_from_id(new_gi->device_id, &device_service);
        }

        // Get the io_service_t for the interface.
        io_service_t interface_service = MACH_PORT_NULL;
        if (error == NULL)
        {
            error = service_get_usb_interface(device_service, interface_number, &interface_service);
        }

        // Get the registry entry ID for the interface.
        if (error == NULL)
        {
            assert(interface_service != MACH_PORT_NULL);
            error = get_id(interface_service, &new_gi->interface_id);
        }

        // Record the fact that we have an ID for the interface.
        if (error == NULL)
        {
            assert(new_gi->interface_id);
            assert(new_gi->interface_id != new_gi->device_id);
            new_gi->has_interface_id = true;
        }

        if (device_service != MACH_PORT_NULL) { IOObjectRelease(device_service); }
        if (interface_service != MACH_PORT_NULL) { IOObjectRelease(interface_service); }
    }

    // Pass the new generic interface to the caller.
    if (error == NULL)
    {
        *gi = new_gi;
        new_gi = NULL;
    }

    libusbp_generic_interface_free(new_gi);
    return error;
}

void libusbp_generic_interface_free(libusbp_generic_interface * gi)
{
    free(gi);
}

libusbp_error * libusbp_generic_interface_copy(
    const libusbp_generic_interface * source,
    libusbp_generic_interface ** dest)
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

    libusbp_error * error = NULL;

    // Allocate the generic interface.
    libusbp_generic_interface * new_gi = NULL;
    if (error == NULL)
    {
        error = generic_interface_allocate(&new_gi);
    }

    // Copy the simple fields.
    if (error == NULL)
    {
        memcpy(new_gi, source, sizeof(libusbp_generic_interface));
    }

    // Pass the generic interface to the caller.
    if (error == NULL)
    {
        *dest = new_gi;
        new_gi = NULL;
    }

    libusbp_generic_interface_free(new_gi);
    return error;
}

uint64_t generic_interface_get_device_id(const libusbp_generic_interface * gi)
{
    assert(gi != NULL);
    return gi->device_id;
}

uint8_t generic_interface_get_interface_number(const libusbp_generic_interface * gi)
{
    assert(gi != NULL);
    return gi->interface_number;
}

bool generic_interface_get_interface_id(const libusbp_generic_interface * gi, uint64_t * id)
{
    assert(gi != NULL);
    if (gi->has_interface_id)
    {
        *id = gi->interface_id;
        return 1;
    }
    else
    {
        *id = 0;
        return 0;
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

    // Some information is being lost here, unfortunately.
    uint64_t idnum = gi->has_interface_id ? gi->interface_id : gi->device_id;

    return iokit_id_to_string(idnum, id);
}

libusbp_error * libusbp_generic_interface_get_os_filename(
    const libusbp_generic_interface * gi,
    char ** filename)
{
    return libusbp_generic_interface_get_os_id(gi, filename);
}


