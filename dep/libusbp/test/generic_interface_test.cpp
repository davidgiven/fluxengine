#include <test_helper.h>

TEST_CASE("generic interface cannot be created from a NULL device")
{
    try
    {
        libusbp::device device;
        libusbp::generic_interface gi(device, 0, true);
        REQUIRE(0);
    }
    catch(const libusbp::error & error)
    {
        REQUIRE(std::string(error.what()) == "Device is null.");
    }
}

static void check_null_gi_error(const libusbp::error & error)
{
    CHECK(error.message() == "Generic interface is null.");
}

TEST_CASE("null generic interface")
{
    libusbp::generic_interface gi;

    SECTION("is not present")
    {
        REQUIRE_FALSE(gi);
    }

    SECTION("is copyable")
    {
        libusbp::generic_interface gi2 = gi;
        REQUIRE_FALSE(gi2);
    }

    SECTION("get_os_id returns an error")
    {
        try
        {
            gi.get_os_id();
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            check_null_gi_error(error);
        }
    }

    SECTION("get_os_filename returns an error")
    {
        try
        {
            gi.get_os_filename();
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            check_null_gi_error(error);
        }
    }
}

#ifdef USE_TEST_DEVICE_A
TEST_CASE("generic interface parameter validation and corner cases")
{
    SECTION("libusbp_generic_interface_create")
    {
        SECTION("complains if the output pointer is NULL")
        {
            try
            {
                libusbp::throw_if_needed(libusbp_generic_interface_create(
                        NULL, 0, true, NULL));
                REQUIRE(0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Generic interface output pointer is null.");
            }
        }

        SECTION("sets the output to NULL if there is an error")
        {
            libusbp_generic_interface * ptr = (libusbp_generic_interface *)-1;
            libusbp::error error(libusbp_generic_interface_create(NULL, 0, true, &ptr));
            REQUIRE((ptr == NULL));
        }
    }

    SECTION("libusbp_generic_interface_copy")
    {
        SECTION("complains if the output pointer is NULL")
        {
            try
            {
                libusbp::throw_if_needed(libusbp_generic_interface_copy(NULL, NULL));
                REQUIRE(0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Generic interface output pointer is null.");
            }
        }

        SECTION("sets the output pointer to NULL when copying NULL")
        {
            libusbp_generic_interface * gi = (libusbp_generic_interface *)-1;
            libusbp::throw_if_needed(libusbp_generic_interface_copy(NULL, &gi));
            REQUIRE((gi == NULL));
        }
    }

    SECTION("libusbp_generic_interface_get_os_id")
    {
        SECTION("complains if the output pointer is NULL")
        {
            try
            {
                libusbp::throw_if_needed(libusbp_generic_interface_get_os_id(NULL, NULL));
                REQUIRE(0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "String output pointer is null.");
            }
        }

        SECTION("sets the output string to NULL if it can't return anything")
        {
            char * s = (char *)-1;
            libusbp::error error(libusbp_generic_interface_get_os_id(NULL, &s));
            REQUIRE((s == NULL));
        }
    }

    SECTION("libusbp_generic_interface_get_os_filename")
    {
        SECTION("complains if the output pointer is NULL")
        {
            try
            {
                libusbp::throw_if_needed(
                    libusbp_generic_interface_get_os_filename(NULL, NULL));
                REQUIRE(0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "String output pointer is null.");
            }
        }

        SECTION("sets the output string to NULL if it can't return anything")
        {
            char * s = (char *)-1;
            libusbp::error error(libusbp_generic_interface_get_os_filename(NULL, &s));
            REQUIRE((s == NULL));
        }
    }
}
#endif

#ifdef USE_TEST_DEVICE_A
static void assert_not_ready(
    const libusbp::device & device, uint8_t interface_number, bool composite)
{
    try
    {
        libusbp::generic_interface gi(device, interface_number, composite);
        CHECK(gi);
        REQUIRE(0);
    }
    catch(const libusbp::error & error)
    {
        if (!error.has_code(LIBUSBP_ERROR_NOT_READY)) { throw; }
    }
}

__attribute__((__unused__))
static void assert_incorrect_driver(
    const libusbp::device & device,
    uint8_t interface_number,
    bool composite,
    std::string driver_name)
{
    std::string expected =
        std::string("Failed to initialize generic interface.  ") +
        "Device is attached to an incorrect driver: "
        + driver_name + ".";

    try
    {
        libusbp::generic_interface gi(device, interface_number, composite);
        REQUIRE(0);
    }
    catch(const libusbp::error & error)
    {
        CHECK_FALSE(error.has_code(LIBUSBP_ERROR_NOT_READY));
        CHECK(error.message() == expected);
    }
}

#ifdef _WIN32
static void assert_no_driver(
    const libusbp::device & device, uint8_t interface_number, bool composite)
{
    try
    {
        libusbp::generic_interface gi(device, interface_number, composite);
        REQUIRE(0);
    }
    catch(const libusbp::error & error)
    {
        CHECK(error.has_code(LIBUSBP_ERROR_NOT_READY));
        CHECK(std::string(error.what()) ==
            "Failed to initialize generic interface.  "
            "Device is not using any driver.");
    }
}
#endif
#endif

#ifdef USE_TEST_DEVICE_A
TEST_CASE("Test Device A generic interface ", "[tdagi]")
{
    libusbp::device device = find_test_device_a();

    SECTION("interface 0 with incorrect hint composite=false")
    {
        #ifdef _WIN32
        // Causes a problem in Windows.
        assert_incorrect_driver(device, 0, false, "usbccgp");
        #else
        // Works fine in Linux, which ignores that hint.
        libusbp::generic_interface gi(device, 0, false);
        #endif
    }

    SECTION("interface 0")
    {
        libusbp::generic_interface gi(device, 0, true);

        SECTION("has a good OS id")
        {
            std::string id = gi.get_os_id();
            #ifdef _WIN32
            REQUIRE(id.substr(0, 12) == "USB\\VID_1FFB");
            #elif defined(__linux__)
            REQUIRE(id.substr(0, 13) == "/sys/devices/");
            #else
            REQUIRE(id.size() > 1);
            for(size_t i = 0; i < id.size(); i++)
            {
                char c = id[i];
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                {
                    throw id + " is a bad OS ID for a generic interface";
                }
            }
            #endif
            REQUIRE(id != device.get_os_id());
        }

        SECTION("has a good OS filename")
        {
            std::string filename = gi.get_os_filename();
            #ifdef _WIN32
            REQUIRE(filename.substr(0, 16) == "\\\\?\\usb#vid_1ffb");
            #elif defined(__linux__)
            REQUIRE(filename.substr(0, 13) == "/dev/bus/usb/");
            #else
            REQUIRE(filename.size() > 0);
            for(size_t i = 0; i < filename.size(); i++)
            {
                char c = filename[i];
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                {
                    throw filename + " is a bad OS ID for a generic interface";
                }
            }
            #endif

            REQUIRE(filename != device.get_os_id());
        }
    }

    SECTION("interface 1 (native interface without Windows drivers)")
    {
        #ifdef _WIN32
        assert_no_driver(device, 1, true);
        #else
        libusbp::generic_interface gi(device, 1, true);
        #endif
    }

    SECTION("interface 2 (serial port)")
    {
        // See comments for libusbp_generic_interface_create for information
        // about why these are different.
        #if defined(_WIN32)
        assert_incorrect_driver(device, 2, true, "usbser");
        #elif defined(__linux__)
        assert_incorrect_driver(device, 2, true, "cdc_acm");
        #elif defined(__APPLE__)
        libusbp::generic_interface(device, 2, true);
        #else
        REQUIRE(0);
        #endif
    }

    SECTION("interface 44 (does not exist)")
    {
        assert_not_ready(device, 44, true);
    }

    SECTION("interfaces can be copied")
    {
        libusbp::generic_interface gi(device, 0, true);
        libusbp::generic_interface gi2 = gi;
        REQUIRE(gi2);
        REQUIRE(gi.get_os_id() == gi2.get_os_id());
        REQUIRE(gi.get_os_filename() == gi2.get_os_filename());
    }
}
#endif
