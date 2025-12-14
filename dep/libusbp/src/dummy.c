
// This file contains failing place-holders to make things compile
// on otherwise unsupported platforms.

#include <libusbp_internal.h>

struct libusbp_device
{
    char* syspath;
    char* serial_number; // may be NULL
    uint16_t product_id;
    uint16_t vendor_id;
    uint16_t revision;
};

static libusbp_error* fail()
{
    return error_create("USB hardware is not supported on this platform");
}

libusbp_error* libusbp_device_copy(
    const libusbp_device* source, libusbp_device** dest)
{
    return fail();
}

libusbp_error* libusbp_generic_interface_create(const libusbp_device* device,
    uint8_t interface_number,
    bool composite __attribute__((unused)),
    libusbp_generic_interface** gi)
{
    return fail();
}

libusbp_error* libusbp_generic_handle_open(
    const libusbp_generic_interface* gi, libusbp_generic_handle** handle)
{
    return fail();
}

void libusbp_device_free(libusbp_device* device) {}

void libusbp_generic_handle_close(libusbp_generic_handle* handle) {}

void libusbp_generic_interface_free(libusbp_generic_interface* gi) {}

libusbp_error* libusbp_device_get_vendor_id(
    const libusbp_device* device, uint16_t* vendor_id)
{
    return fail();
}

libusbp_error* libusbp_device_get_product_id(
    const libusbp_device* device, uint16_t* product_id)
{
    return fail();
}

libusbp_error* libusbp_device_get_serial_number(
    const libusbp_device* device, char** serial_number)
{
    return fail();
}

libusbp_error* libusbp_write_pipe(libusbp_generic_handle* handle,
    uint8_t pipe_id,
    const void* data,
    size_t size,
    size_t* transferred)
{
    return fail();
}

libusbp_error* libusbp_read_pipe(libusbp_generic_handle* handle,
    uint8_t pipe_id,
    void* data,
    size_t size,
    size_t* transferred)
{
    return fail();
}

libusbp_error* libusbp_serial_port_create(const libusbp_device* device,
    uint8_t interface_number,
    bool composite,
    libusbp_serial_port** port)
{
    return fail();
}

libusbp_error* libusbp_serial_port_get_name(
    const libusbp_serial_port* port, char** name)
{
    return fail();
}

void libusbp_serial_port_free(libusbp_serial_port* port) {}

libusbp_error* libusbp_list_connected_devices(
    libusbp_device*** device_list, size_t* device_count)
{
    return fail();
}
