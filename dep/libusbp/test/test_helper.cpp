#include <test_helper.h>

libusbp::device find_by_vendor_id_product_id(uint16_t vendor_id, uint16_t product_id)
{
    std::vector<libusbp::device> list = libusbp::list_connected_devices();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        libusbp::device device = *it;
        if (device.get_vendor_id() == vendor_id && device.get_product_id() == product_id)
        {
            return device;
        }
    }
    throw "Device not found.";
}

libusbp::device find_test_device_a()
{
    return find_by_vendor_id_product_id(0x1FFB, 0xDA01);
}

libusbp::device find_test_device_b()
{
    return find_by_vendor_id_product_id(0x1FFB, 0xDA02);
}
