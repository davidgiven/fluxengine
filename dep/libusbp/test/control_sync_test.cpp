#include <test_helper.h>

TEST_CASE("control transfer corner cases")
{
    SECTION("sets transferred to zero if possible")
    {
        size_t transferred = 1;
        libusbp::error error(libusbp_control_transfer(NULL,
                0, 0, 0, 0, NULL, 0, &transferred));
        REQUIRE(transferred == 0);
    }
}

#ifdef USE_TEST_DEVICE_A

TEST_CASE("control transfers (synchronous) for Test Device A", "[ctstda]")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle gih(gi);
    size_t transferred = 0xF12F;
    gih.set_timeout(0, 300);

    SECTION("request without a data stage")
    {
        // Turn on the LED.
        gih.control_transfer(0x40, 0x90, 1, 0, NULL, 0, &transferred);
        REQUIRE(transferred == 0);
    }

    SECTION("writing and reading data")
    {
        char buffer1[40] = "hello there";
        size_t size = strlen(buffer1) + 1;

        // Transfer data to the device.
        gih.control_transfer(0x40, 0x92, 0, 0, buffer1, size, &transferred);
        REQUIRE(transferred == size);

        // Read the data back.
        char buffer2[40];
        gih.control_transfer(0xC0, 0x91, 0, 12, buffer2, 20, &transferred);
        REQUIRE(transferred == size);
        REQUIRE(std::string(buffer2) == buffer1);
    }

    SECTION("invalid request")
    {
        try
        {
            gih.control_transfer(0x40, 0x48, 0, 0);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            const char * expected =
                "Control transfer failed.  "
                "The request was invalid or there was an I/O problem.  "
                #if defined(_WIN32)
                "Windows error code 0x1f."
                #elif defined(__linux__)
                "Error code 32."
                #elif defined(__APPLE__)
                "Error code 0xe000404f."
                #endif
                ;
            REQUIRE(std::string(error.what()) == expected);
            REQUIRE(error.has_code(LIBUSBP_ERROR_STALL));
        }
    }
}

TEST_CASE("control transfers that time out for Test Device A")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle gih(gi);
    size_t transferred = 0xFFFF;
    gih.set_timeout(0, 1);

    // Figure out what delay is necessary to trigger a control transfer timeout.
    #ifdef __APPLE__
    // Mac OS X seems to have really inaccurate timers and it does not detect a
    // timeout unless the device delays for over roughly one second.  To detect it
    // reliably, the delay needs to be even longer, definitely slows down the tests
    // and makes our device not compliant with the USB specification during that
    // time, because it cannot response to SETUP packets.
    const uint32_t required_delay = 2000;
    #else
    const uint32_t required_delay = 100;
    #endif

    std::string timeout_message = "Control transfer failed.  "
        "The operation timed out.  "
        #if defined(_WIN32)
        "Windows error code 0x79."
        #elif defined(__linux__)
        "Error code 110."
        #elif defined(__APPLE__)
        "Error code 0xe0004051."
        #endif
    ;

    SECTION("write request that times out")
    {
        size_t transferred = 0xFFFF;
        char buffer[] = "hi";
        try
        {
            gih.control_transfer(0x40, 0x92, required_delay, 0,
                buffer, sizeof(buffer), &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) == timeout_message);
            REQUIRE(error.has_code(LIBUSBP_ERROR_TIMEOUT));
            REQUIRE(transferred == 0);
        }
    }

    SECTION("read request that times out")
    {
        size_t transferred = 0xFFFF;
        char buffer[3];
        try
        {
            gih.control_transfer(0xC0, 0x91, required_delay, 0,
                buffer, sizeof(buffer), &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) == timeout_message);
            REQUIRE(error.has_code(LIBUSBP_ERROR_TIMEOUT));
            REQUIRE(transferred == 0);
        }
    }

    #ifdef _WIN32
    SECTION("write request with no data stage that times out (no error raised)")
    {
        // For some reason, WinUSB does not seem to report timeouts for a
        // control transfer with no data stage that times out, which is bad.  I
        // am not sure why, because it used to work.
        gih.control_transfer(0x40, 0x92, required_delay, 0, NULL, 0, &transferred);
    }
    #else

    SECTION("write request with no data stage that times out (error raised)")
    {
        try
        {
            gih.control_transfer(0x40, 0x92, required_delay, 0, NULL, 0, &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) == timeout_message);
            REQUIRE(error.has_code(LIBUSBP_ERROR_TIMEOUT));
            REQUIRE(transferred == 0);
        }
    }
    #endif
}

#endif


#ifdef USE_TEST_DEVICE_B

TEST_CASE("control transfers (synchronous) for Test Device B", "[ctstda]")
{
    libusbp::device device = find_test_device_b();
    libusbp::generic_interface gi(device, 0, false);
    libusbp::generic_handle gih(gi);
    size_t transferred;
    gih.set_timeout(0, 300);

    SECTION("request without a data stage")
    {
        // Turn on the LED.
        gih.control_transfer(0x40, 0x90, 1, 0, NULL, 0, &transferred);
        REQUIRE(transferred == 0);
    }

    SECTION("writing and reading data")
    {
        char buffer1[40] = "hello there";
        size_t size = strlen(buffer1) + 1;

        // Transfer data to the device.
        gih.control_transfer(0x40, 0x92, 0, 0, buffer1, size, &transferred);
        REQUIRE(transferred == size);

        // Read the data back.
        char buffer2[40];
        gih.control_transfer(0xC0, 0x91, 0, 12, buffer2, 20, &transferred);
        REQUIRE(transferred == size);
        REQUIRE(std::string(buffer2) == buffer1);
    }
}

#endif
