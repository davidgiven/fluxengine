#include <test_helper.h>

static void check_null_device_error(const libusbp::error & error)
{
    REQUIRE(error.message() == "Device is null.");
}

TEST_CASE("null device")
{
    libusbp::device device;

    SECTION("is not present")
    {
        REQUIRE_FALSE(device);
    }

    SECTION("can be copied")
    {
        libusbp::device device2 = device;
        REQUIRE_FALSE(device2);
    }

    SECTION("cannot return a vendor ID")
    {
        try
        {
            device.get_vendor_id();
        }
        catch(const libusbp::error & error)
        {
            check_null_device_error(error);
        }
    }

    SECTION("cannot return a product ID")
    {
        try
        {
            device.get_product_id();
        }
        catch(const libusbp::error & error)
        {
            check_null_device_error(error);
        }
    }

    SECTION("cannot return a revision")
    {
        try
        {
            device.get_revision();
        }
        catch(const libusbp::error & error)
        {
            check_null_device_error(error);
        }
    }

    SECTION("cannot return a serial number")
    {
        try
        {
            device.get_serial_number();
        }
        catch(const libusbp::error & error)
        {
            check_null_device_error(error);
        }
    }

    SECTION("cannot return an OS id")
    {
        try
        {
            device.get_os_id();
        }
        catch(const libusbp::error & error)
        {
            check_null_device_error(error);
        }
    }
}

TEST_CASE("device parameter checks and corner cases")
{
    SECTION("libusbp_device_copy complains about a null output pointer")
    {
        libusbp::error error(libusbp_device_copy(NULL, NULL));
        REQUIRE(error.message() == "Device output pointer is null.");
    }

    SECTION("libusbp_device_copy sets the output pointer to 0 by default")
    {
        void * p = &p;
        libusbp::error error(libusbp_device_copy(NULL, (libusbp_device**)&p));
        REQUIRE((p == NULL));
    }

    SECTION("libusbp_device_get_vendor_id complains about a null output pointer")
    {
        libusbp::error error(libusbp_device_get_vendor_id(NULL, NULL));
        REQUIRE(error.message() == "Vendor ID output pointer is null.");
    }

    SECTION("libusbp_device_get_vendor_id sets the output to 0 by default")
    {
        uint16_t x = 1;
        libusbp::error error(libusbp_device_get_vendor_id(NULL, &x));
        REQUIRE(x == 0);
    }

    SECTION("libusbp_device_get_product_id complains about a null output pointer")
    {
        libusbp::error error(libusbp_device_get_product_id(NULL, NULL));
        REQUIRE(error.message() == "Product ID output pointer is null.");
    }

    SECTION("libusbp_device_get_product_id sets the output to 0 by default")
    {
        uint16_t x = 1;
        libusbp::error error(libusbp_device_get_product_id(NULL, &x));
        REQUIRE(x == 0);
    }

    SECTION("libusbp_device_get_revision complains about a null output pointer")
    {
        libusbp::error error(libusbp_device_get_revision(NULL, NULL));
        REQUIRE(error.message() == "Device revision output pointer is null.");
    }

    SECTION("libusbp_device_get_revision sets the output to 0 by default")
    {
        uint16_t x = 1;
        libusbp::error error(libusbp_device_get_revision(NULL, &x));
        REQUIRE(x == 0);
    }

    SECTION("libusbp_device_get_serial_number complains about a null output pointer")
    {
        libusbp::error error(libusbp_device_get_serial_number(NULL, NULL));
        REQUIRE(error.message() == "Serial number output pointer is null.");
    }

    SECTION("libusbp_device_get_serial_number sets the output to NULL by default")
    {
        void * p = &p;
        libusbp::error error(libusbp_device_get_serial_number(NULL, (char **)&p));
        REQUIRE((p == NULL));
    }

    SECTION("libusbp_device_get_os_id complains about a null output pointer")
    {
        libusbp::error error(libusbp_device_get_os_id(NULL, NULL));
        REQUIRE(error.message() == "Device OS ID output pointer is null.");
    }

    SECTION("libusbp_device_get_os_id sets the output to NULL by default")
    {
        void * p = &p;
        libusbp::error error(libusbp_device_get_os_id(NULL, (char **)&p));
        REQUIRE((p == NULL));
    }
}

TEST_CASE("basic checks on all devices", "[device_basic]")
{
    const bool print_devices = false;

    std::vector<libusbp::device> list = libusbp::list_connected_devices();
    for (auto it = list.begin(); it != list.end(); ++it)
    {
        libusbp::device device = *it;

        uint16_t vendor_id = device.get_vendor_id();
        uint16_t product_id = device.get_product_id();
        uint16_t revision = device.get_revision();

        std::string serial;
        try
        {
            serial = device.get_serial_number();
        }
        catch(const libusbp::error & error)
        {
            if (!error.has_code(LIBUSBP_ERROR_NO_SERIAL_NUMBER)) { throw; }
        }

        std::string id = device.get_os_id();
        CHECK_FALSE(id.empty());

        if (print_devices)
        {
            printf("Device: %04x:%04x:%04x %-32s %s\n",
                vendor_id, product_id, revision, serial.c_str(), id.c_str());
        }
    }
}

#ifdef USE_TEST_DEVICE_A
TEST_CASE("Test Device A", "[tda]")
{
    libusbp::device device = find_test_device_a();

    SECTION("present")
    {
        REQUIRE(device);
    }

    SECTION("revision code")
    {
        // If this test fails, you should probably update
        // your Test Device A with the latest firmware.
        REQUIRE(device.get_revision() == 0x0007);
    }

    SECTION("device instance id")
    {
        std::string id = device.get_os_id();
        REQUIRE_FALSE(id.empty());
        #ifdef _WIN32
        REQUIRE(id.find("USB") == 0);
        #endif
    }

    SECTION("serial number")
    {
        std::string serial_number = device.get_serial_number();
        CHECK(serial_number.size() == 11);
        CHECK(serial_number[2] == '-');
    }
}
#endif

#ifdef USE_TEST_DEVICE_B
TEST_CASE("Test Device B", "[tdb]")
{
    libusbp::device device = find_test_device_b();

    SECTION("present")
    {
        REQUIRE(device);
    }

    SECTION("revision code")
    {
        // If this test fails, you should probably update
        // your Test Device B with the latest firmware.
        REQUIRE(device.get_revision() == 0x0007);
    }
}
#endif
