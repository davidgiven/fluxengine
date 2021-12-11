#include <test_helper.h>

#ifdef USE_TEST_DEVICE_A
TEST_CASE("read_pipe parameter checking")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);
    const uint8_t pipe = 0x82;
    size_t transferred = 0xFFFF;

    SECTION("sets transferred to zero if possible")
    {
        size_t transferred = 1;
        libusbp::error error(libusbp_read_pipe(NULL,
                0, NULL, 0, &transferred));
        REQUIRE(transferred == 0);
    }

    SECTION("requires the size to be non-zero")
    {
        // The corner case of the transfer size being zero seems to put the USB
        // drivers in Linux in a weird state, so let's just not allow it.
        try
        {
            handle.read_pipe(pipe, NULL, 0, &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) ==
                "Failed to read from pipe.  "
                "Transfer size 0 is not allowed.");
            REQUIRE(transferred == 0);
        }
    }

    SECTION("requires the buffer to be non-NULL")
    {
        try
        {
            handle.read_pipe(pipe, NULL, 2, &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) ==
                "Failed to read from pipe.  "
                "Buffer is null.");
            REQUIRE(transferred == 0);
        }
    }

    SECTION("requires the direction bit to be correct")
    {
        try
        {
            uint8_t buffer[5];
            handle.read_pipe(0x02, buffer, sizeof(buffer), &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) ==
                "Failed to read from pipe.  "
                "Invalid pipe ID 0x02.");
            REQUIRE(transferred == 0);
        }
    }

    SECTION("checks the size")
    {
        uint8_t buffer[5];

        #if defined(_WIN32)
        size_t too_large_size = (size_t)ULONG_MAX + 1;
        #elif defined(__linux__)
        size_t too_large_size = (size_t)UINT_MAX + 1;
        #elif defined(__APPLE__)
        size_t too_large_size = (size_t)UINT32_MAX + 1;
        #else
        #error add a case for this OS
        #endif

        if (too_large_size == 0) { return; }

        try
        {
            handle.read_pipe(pipe, buffer, too_large_size, NULL);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == "Failed to read from pipe.  Transfer size is too large.");
        }
    }
}

TEST_CASE("read_pipe (synchronous) on an interrupt endpoint ", "[rpi]")
{
    // We assume that if read_pipe works on an interrupt endpoint, it will also
    // work on a bulk endpoint because of the details of the underlying APIs
    // that libusbp uses.

    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);
    const uint8_t pipe = 0x82;
    size_t transferred = 0xFFFF;
    const size_t packet_size = 5;

    // Unpause the ADC if it was paused by some previous test.
    handle.control_transfer(0x40, 0xA0, 0, 0);

    SECTION("can read one packet")
    {
        uint8_t buffer[packet_size];
        handle.read_pipe(pipe, buffer, sizeof(buffer), &transferred);
        REQUIRE(transferred == packet_size);
        REQUIRE(buffer[4] == 0xAB);
    }

    SECTION("can read two packets")
    {
        uint8_t buffer[packet_size * 2];
        handle.read_pipe(pipe, buffer, sizeof(buffer), &transferred);
        REQUIRE(transferred == packet_size * 2);
        REQUIRE(buffer[4] == 0xAB);
        REQUIRE(buffer[4 + packet_size] == 0xAB);
    }

    SECTION("can read without returning the size transferred")
    {
        // But this is bad; you should be checking how many bytes are
        // transferred.
        uint8_t buffer[packet_size];
        handle.read_pipe(pipe, buffer, sizeof(buffer), NULL);
        REQUIRE(buffer[4] == 0xAB);
    }

    SECTION("overflows when transfer size is not a multiple of the packet size")
    {
        #ifdef VBOX_LINUX_ON_WINDOWS
        // This test fails and then puts the USB device into a weird state
        // if run on Linux inside VirtualBox on a Windows host.
        std::cerr << "Skipping synchronous IN pipe overflow test.\n";
        return;
        #endif

        uint8_t buffer[packet_size + 1];
        size_t transferred;
        try
        {
            handle.read_pipe(pipe, buffer, sizeof(buffer), &transferred);
        }
        catch(const libusbp::error & error)
        {
            const char * expected =
                "Failed to read from pipe.  "
                "The transfer overflowed.  "
                #if defined(_WIN32)
                #elif defined(__linux__)
                "Error code 75."
                #elif defined(__APPLE__)
                "Error code 0xe00002e8."
                #endif
                ;
            REQUIRE(error.message() == expected);

            #ifdef __APPLE__
            REQUIRE(transferred == packet_size + 1);
            #else
            REQUIRE(transferred == 0);
            #endif
        }
    }


    #ifdef __APPLE__
    SECTION("does not have an adjustable timeout for interrupt endpoints")
    {
        handle.set_timeout(pipe, 1);

        uint8_t buffer[packet_size];
        try
        {
            handle.read_pipe(pipe, buffer, sizeof(buffer), NULL);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            // A copy of this error message is in the README, so be sure
            // to update the README if you have to update this.
            REQUIRE(error.message() == "Failed to read from pipe.  "
                "(iokit/common) invalid argument.  Error code 0xe00002c2.");
        }
    }
    #else
    SECTION("has an adjustable timeout")
    {
        // Pause the ADC for 50 ms, then try to read data from it with a 1 ms
        // timeout and observe that it fails.

        handle.set_timeout(pipe, 1);
        handle.control_transfer(0x40, 0xA0, 50, 0);

        uint8_t buffer[packet_size];
        try
        {
            // We use two extra reads to clear out packets that might have been
            // queued on the device before we stopped the pipe.
            handle.read_pipe(pipe, buffer, sizeof(buffer), &transferred);
            handle.read_pipe(pipe, buffer, sizeof(buffer), &transferred);

            // This last read will hopefully time out.
            handle.read_pipe(pipe, buffer, sizeof(buffer), &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            const char * expected =
                "Failed to read from pipe.  "
                "The operation timed out.  "
                #ifdef _WIN32
                "Windows error code 0x79."
                #elif defined(__linux__)
                "Error code 110."
                #endif
                ;
            REQUIRE(std::string(error.what()) == expected);
            REQUIRE(error.has_code(LIBUSBP_ERROR_TIMEOUT));

            #ifdef _WIN32
            // Sometimes WinUSB transfers 5 bytes successfully but it still
            // reports a timeout.  This seems to depend on what computer you are
            // running the tests on; on some computers it happens about half the
            // time, and on some computers it never happens.  When this was
            // first observed, libusbp was not using RAW_IO mode.
            REQUIRE((transferred == 0 || transferred == 5));
            #else
            REQUIRE(transferred == 0);
            #endif
        }

        // Do the same thing with a higher timeout and observe that it works.
        handle.set_timeout(pipe, 500);
        handle.control_transfer(0x40, 0xA0, 50, 0);
        for (uint8_t i = 0; i < 3; i++)
        {
            handle.read_pipe(pipe, buffer, sizeof(buffer), &transferred);
            REQUIRE(transferred == 5);
        }
    }
    #endif
}
#endif
