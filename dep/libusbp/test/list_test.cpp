/* Tests the functions we provide for finding/listing devices. */

#include <test_helper.h>

TEST_CASE("list_connected_device (C++)")
{
    #ifdef USE_TEST_DEVICE_A
    SECTION("can find Test Device A")
    {
        libusbp::device found_device;
        std::vector<libusbp::device> list = libusbp::list_connected_devices();
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            libusbp::device device = *it;
            if (!device)
            {
                throw "A null device was returned by list_connected_devices.";
            }
            if (device.get_vendor_id() == 0x1FFB && device.get_product_id() == 0xDA01)
            {
                return;
            }
        }
        throw "Test Device A not found.";
    }
    #endif
}

TEST_CASE("list_connected_devices (C)")
{
    libusbp_device ** list = NULL;
    size_t device_count = 4444;

    SECTION("gives a null-terminated list")
    {
        libusbp_error * error = libusbp_list_connected_devices(&list, &device_count);
        if (error != NULL) { throw libusbp::error(error); }
        for (size_t i = 0; i < device_count; i++)
        {
            CHECK(list[i]);
        }
        CHECK_FALSE(list[device_count]);
    }

    SECTION("does not crash if called with a NULL device_list argument")
    {
        libusbp::error error(libusbp_list_connected_devices(NULL, &device_count));
        REQUIRE(error.message() == "Device list output pointer is null.");
        REQUIRE(device_count == 0);
    }

    SECTION("does not complain if called with a NULL device_count argument")
    {
        libusbp_error * error = libusbp_list_connected_devices(&list, NULL);
        if (error != NULL) { throw libusbp::error(error); }
    }

    if (list != NULL)
    {
        libusbp_device ** device = list;
        while(*device)
        {
            libusbp_device_free(*device);
            device++;
        }
        libusbp_list_free(list);
    }
}

TEST_CASE("find_device_with_vid_pid (C++)")
{
    #ifdef USE_TEST_DEVICE_A
    SECTION("can find Test Device A")
    {
        libusbp::device device = libusbp::find_device_with_vid_pid(0x1FFB, 0xDA01);
        REQUIRE(device);
        REQUIRE(device.get_vendor_id() == 0x1FFB);
        REQUIRE(device.get_product_id() == 0xDA01);
    }
    #endif

    SECTION("returns a null device if the specified one is not found")
    {
        // It's not an error when a device you are looking for isn't connected
        // to the system, it's normal and expected, and you should be checking
        // for it and handling it explicitly.
        libusbp::device device = libusbp::find_device_with_vid_pid(0xABCD, 0x1234);
        REQUIRE_FALSE(device);
    }
}

TEST_CASE("find_device_with_vid_pid (C)")
{
    SECTION("complains if the output pointer is NULL")
    {
        libusbp::error error(libusbp_find_device_with_vid_pid(0xABCD, 0x1234, NULL));
        REQUIRE(error.message() == "Device output pointer is null.");
    }
}
