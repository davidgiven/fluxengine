#include <libusbp_internal.h>

struct libusbp_serial_port
{
    // A sysfs path like "/sys/devices/pci0000:00/0000:00:06.0/usb1/1-2/1-2:1.0/tty/ttyACM0".
    // It corresponds to a udev device with subsystem "tty".
    char * syspath;

    // A port filename like "/dev/ttyACM0".
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

    // Get the syspath of the physical device.
    char * new_device_syspath = NULL;
    if (error == NULL)
    {
        error = libusbp_device_get_os_id(device, &new_device_syspath);
    }

    // Get a udev context.
    struct udev * new_udev = NULL;
    if (error == NULL)
    {
        error = udevw_create_context(&new_udev);
    }

    // Get the USB interface device.
    struct udev_device * new_interface_dev = NULL;
    if (error == NULL)
    {
        error = udevw_get_interface(new_udev, new_device_syspath,
            interface_number, &new_interface_dev);
    }

    // Get the tty device.
    struct udev_device * new_tty_dev = NULL;
    if (error == NULL)
    {
        error = udevw_get_tty(new_udev, new_interface_dev, &new_tty_dev);
    }

    // Get the syspath of the tty device.
    if (error == NULL)
    {
        error = udevw_get_syspath_copy(new_tty_dev, &new_port->syspath);
    }

    // Get the port name (e.g. /dev/ttyACM0)
    const char * port_name = NULL;
    if (error == NULL)
    {
        port_name = udev_device_get_property_value(new_tty_dev, "DEVNAME");
        if (port_name == NULL)
        {
            error = error_create("The DEVNAME property does not exist.");
        }
    }

    // Copy the port name to the new serial port object.
    if (error == NULL)
    {
        error = string_copy(port_name, &new_port->port_name);
    }

    // Pass the new object to the caller.
    if (error == NULL)
    {
        *port = new_port;
        new_port = NULL;
    }

    if (new_tty_dev != NULL) { udev_device_unref(new_tty_dev); }
    if (new_interface_dev != NULL) { udev_device_unref(new_interface_dev); }
    if (new_udev != NULL) { udev_unref(new_udev); }
    libusbp_string_free(new_device_syspath);
    libusbp_serial_port_free(new_port);

    return error;
}

void libusbp_serial_port_free(libusbp_serial_port * port)
{
    if (port != NULL)
    {
        libusbp_string_free(port->syspath);
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

    // Copy the syspath.
    if (error == NULL)
    {
        error = string_copy(source->syspath, &new_port->syspath);
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
