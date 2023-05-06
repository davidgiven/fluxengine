#include <libusbp_internal.h>

struct libusbp_serial_port
{
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

    libusbp_error * error = NULL;

    libusbp_serial_port * new_port = NULL;
    if (error == NULL)
    {
        new_port = calloc(1, sizeof(libusbp_serial_port));
        if (new_port == NULL)
        {
            error = &error_no_memory;
        }
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

    io_iterator_t iterator = MACH_PORT_NULL;
    if (error == NULL)
    {
        kern_return_t result = IORegistryEntryCreateIterator(
            device_service, kIOServicePlane, kIORegistryIterateRecursively, &iterator);
        if (result != KERN_SUCCESS)
        {
            error = error_create_mach(result, "Failed to get recursive iterator.");
        }
    }

    io_service_t serial_service = MACH_PORT_NULL;
    int32_t current_interface = -1;
    int32_t last_acm_control_interface_with_no_port = -1;
    int32_t last_acm_data_interface = -1;
    while (error == NULL)
    {
        io_service_t service = IOIteratorNext(iterator);
        if (service == MACH_PORT_NULL) { break; }

        if (IOObjectConformsTo(service, kIOUSBHostInterfaceClassName))
        {
            error = get_int32(service, CFSTR("bInterfaceNumber"), &current_interface);
        }
        else if (IOObjectConformsTo(service, "AppleUSBACMControl"))
        {
            last_acm_control_interface_with_no_port = current_interface;
        }
        else if (IOObjectConformsTo(service, "AppleUSBACMData"))
        {
            last_acm_data_interface = current_interface;
        }
        else if (IOObjectConformsTo(service, kIOSerialBSDServiceValue))
        {
          int32_t fixed_interface = current_interface;
          if (last_acm_data_interface == current_interface &&
              last_acm_control_interface_with_no_port >= 0)
          {
              // We found an ACM control interface with no serial port, then
              // an ACM data interface with a serial port.  For consistency with
              // other operating systems, we will consider this serial port to
              // actually be associated with the control interface instead of the
              // data interface.
              fixed_interface = last_acm_control_interface_with_no_port;
          }
          last_acm_control_interface_with_no_port = -1;

          if (fixed_interface == interface_number)
          {
              // We found the serial port the user is looking for.
              serial_service = service;
              break;
          }
        }
        IOObjectRelease(service);
    }

    if (error == NULL && serial_service == MACH_PORT_NULL)
    {
        error = error_create("Could not find entry with class IOSerialBSDClient.");
        error = error_add_code(error, LIBUSBP_ERROR_NOT_READY);
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
    if (iterator != MACH_PORT_NULL) { IOObjectRelease(iterator); }
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
    libusbp_serial_port * new_port = NULL;
    if (error == NULL)
    {
        new_port = calloc(1, sizeof(libusbp_serial_port));
        if (new_port == NULL)
        {
            error = &error_no_memory;
        }
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
