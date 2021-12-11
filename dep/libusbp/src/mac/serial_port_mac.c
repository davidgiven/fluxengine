#include <libusbp_internal.h>

struct libusbp_serial_port
{
    // The I/O Registry ID of the IOBSDSerialClient.
    uint64_t id;

    // A port filename like "/dev/cu.usbmodemFD123".
    char * port_name;
};

libusbp_error * libusbp_serial_port_create(
    const libusbp_device * device,
    uint8_t interface_number,
    bool composite,
    libusbp_serial_port ** port)
{
    LIBUSBP_UNUSED(composite);

    if (port == NULL)
    {
        return error_create("Serial port output pointer is null.");
    }

    *port = NULL;

    if (device == NULL)
    {
        return error_create("Device is null.");
    }

    // Add one to the interface number because that is what we need for the
    // typical case: The user specifies the lower of the two interface numbers,
    // which corresponds to the control interface of a CDC ACM device.  We
    // actually need the data interface because that is the one that the
    // IOSerialBSDClient lives under.  If this +1 causes any problems, it is
    // easy for the user to address it using an an ifdef.  Also, we might make
    // this function more flexible in the future if we need to handle different
    // types of serial devices with different drivers or interface layouts.
    interface_number += 1;

    libusbp_error * error = NULL;

    libusbp_serial_port * new_port = calloc(1, sizeof(libusbp_serial_port));

    if (new_port == NULL)
    {
        error = &error_no_memory;
    }

    // Get the ID for the physical device.
    uint64_t device_id;
    if (error == NULL)
    {
        device_id = device_get_id(device);
    }

    // Get an io_service_t for the physical device.
    io_service_t device_service = MACH_PORT_NULL;
    if (error == NULL)
    {
        error = service_get_from_id(device_id, &device_service);
    }

    // Get an io_service_t for the interface.
    io_service_t interface_service = MACH_PORT_NULL;
    if (error == NULL)
    {
        error = service_get_usb_interface(device_service, interface_number, &interface_service);
    }

    // Get an io_service_t for the IOSerialBSDClient
    io_service_t serial_service = MACH_PORT_NULL;
    if (error == NULL)
    {
        error = service_get_child_by_class(interface_service,
            kIOSerialBSDServiceValue, &serial_service);
    }

    // Get the port name.
    if (error == NULL)
    {
        error = get_string(serial_service, CFSTR(kIOCalloutDeviceKey), &new_port->port_name);
    }

    // Pass the new object to the caller.
    if (error == NULL)
    {
        *port = new_port;
        new_port = NULL;
    }

    if (serial_service != MACH_PORT_NULL) { IOObjectRelease(serial_service); }
    if (interface_service != MACH_PORT_NULL) { IOObjectRelease(interface_service); }
    if (device_service != MACH_PORT_NULL) { IOObjectRelease(device_service); }
    libusbp_serial_port_free(new_port);

    return error;
}

void libusbp_serial_port_free(libusbp_serial_port * port)
{
    if (port != NULL)
    {
        libusbp_string_free(port->port_name);
        free(port);
    }
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

    libusbp_error * error = NULL;

    // Allocate memory for the new object.
    libusbp_serial_port * new_port = calloc(1, sizeof(libusbp_serial_port));

    if (new_port == NULL)
    {
        error = &error_no_memory;
    }

    // Copy the port name.
    if (error == NULL)
    {
        error = string_copy(source->port_name, &new_port->port_name);
    }

    // Pass the new object to the caller.
    if (error == NULL)
    {
        *dest = new_port;
        new_port = NULL;
    }

    libusbp_serial_port_free(new_port);
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
