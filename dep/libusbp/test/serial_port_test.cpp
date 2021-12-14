#include <test_helper.h>

TEST_CASE("serial port cannot be created from a NULL device")
{
    try
    {
        libusbp::device device;
        libusbp::serial_port sp(device, 0, true);
        REQUIRE(0);
    }
    catch(const libusbp::error & error)
    {
        REQUIRE(error.message() == "Device is null.");
    }
}

static void check_null_sp_error(const libusbp::error & error)
{
    CHECK(error.message() == "Serial port is null.");
}

TEST_CASE("null serial port")
{
    libusbp::serial_port sp;

    SECTION("is not present")
    {
        REQUIRE_FALSE(sp);
    }

    SECTION("is copyable")
    {
        libusbp::serial_port sp2 = sp;
        REQUIRE_FALSE(sp2);
    }

    SECTION("get_name returns an error")
    {
        try
        {
            sp.get_name();
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            check_null_sp_error(error);
        }
    }
}

TEST_CASE("serial port parameter validation and corner cases")
{
    SECTION("libusbp_serial_port_create")
    {
        SECTION("complains if the output pointer is NULL")
        {
            try
            {
                libusbp::throw_if_needed(libusbp_serial_port_create(
                        NULL, 0, true, NULL));
                REQUIRE(0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Serial port output pointer is null.");
            }
        }

        SECTION("sets the output to NULL if there is an error")
        {
            libusbp_serial_port * ptr = (libusbp_serial_port *)-1;
            libusbp::error error(libusbp_serial_port_create(NULL, 0, true, &ptr));
            REQUIRE((ptr == NULL));
        }
    }

    SECTION("libusbp_serial_port_copy")
    {
        SECTION("complains if the output pointer is NULL")
        {
            try
            {
                libusbp::throw_if_needed(libusbp_serial_port_copy(NULL, NULL));
                REQUIRE(0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Serial port output pointer is null.");
            }
        }

        SECTION("sets the output pointer to NULL when copying NULL")
        {
            libusbp_serial_port * sp = (libusbp_serial_port *)-1;
            libusbp::throw_if_needed(libusbp_serial_port_copy(NULL, &sp));
            REQUIRE((sp == NULL));
        }
    }

    SECTION("libusbp_serial_port_get_name")
    {
        SECTION("complains if the output pointer is NULL")
        {
            try
            {
                libusbp::throw_if_needed(libusbp_serial_port_get_name(NULL, NULL));
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
            libusbp::error error(libusbp_serial_port_get_name(NULL, &s));
            REQUIRE((s == NULL));
        }
    }
}

#ifdef USE_TEST_DEVICE_A
TEST_CASE("test device A serial port")
{
    libusbp::device device = find_test_device_a();

    SECTION("interface 2")
    {
        libusbp::serial_port sp(device, 2, true);

        SECTION("has a name that looks good")
        {
            std::string name = sp.get_name();

            #if defined(_WIN32)
            REQUIRE(name.substr(0, 3) == "COM");
            REQUIRE(name.size() >= 4);
            #elif defined(__linux__)
            REQUIRE(name.substr(0, 11) == "/dev/ttyACM");
            REQUIRE(name.size() >= 11);
            #elif defined(__APPLE__)
            REQUIRE(name.substr(0, 16) == "/dev/cu.usbmodem");
            REQUIRE(name.size() >= 17);
            #else
            REQUIRE(0);
            #endif
        }

    }

    SECTION("incorrect parameters interface=0 and composite=false")
    {
        try
        {
            libusbp::serial_port sp(device, 0, true);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            #if defined(_WIN32)
            REQUIRE(error.message() == "The PortName key was not found.");
            #elif defined(__linux__)
            REQUIRE(error.message() == "Could not find tty device.");
            #elif defined(__APPLE__)
            REQUIRE(error.message() == "Could not find entry with class IOSerialBSDClient.");
            #else
            REQUIRE(error.message() == "?");
            #endif
        }
    }

    SECTION("serial ports can be copied")
    {
        libusbp::serial_port sp(device, 2, true);
        libusbp::serial_port sp2 = sp;
        REQUIRE(sp2.get_name() == sp.get_name());
    }
}
#endif
