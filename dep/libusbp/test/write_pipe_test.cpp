#include <test_helper.h>

#ifdef USE_TEST_DEVICE_A
TEST_CASE("write_pipe parameter checking")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);
    const uint8_t pipe = 0x03;
    size_t transferred = 0xFFFF;

    SECTION("sets transferred to zero if possible")
    {
        size_t transferred = 1;
        libusbp::error error(libusbp_write_pipe(NULL,
            0, NULL, 0, &transferred));
        REQUIRE(transferred == 0);
    }

    SECTION("requires the buffer to be non-NULL")
    {
        try
        {
            handle.write_pipe(pipe, NULL, 2, &transferred);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) ==
                "Failed to write to pipe.  "
                "Buffer is null.");
            REQUIRE(transferred == 0);
        }
    }

    SECTION("requires the direction bit to be correct")
    {
        try
        {
            uint8_t buffer[5];
            handle.write_pipe(0x83, buffer, sizeof(buffer), &transferred);
            REQUIRE(0);
        }
        catch (const libusbp::error & error)
        {
            REQUIRE(std::string(error.what()) ==
                "Failed to write to pipe.  "
                "Invalid pipe ID 0x83.");
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
            handle.write_pipe(pipe, buffer, too_large_size, NULL);
            REQUIRE(0);
        }
        catch (const libusbp::error & error)
        {
            REQUIRE(error.message() == "Failed to write to pipe.  "
              "Transfer size is too large.");
        }
    }
}

TEST_CASE("write_pipe (synchronous) on a bulk endpoint ", "[wpi]")
{
    // We assume that if write_pipe works on a bulk endpoint, it will also
    // work on an interrupt endpoint because of the details of the underlying
    // APIs that libusbp uses.

    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);
    const uint8_t pipe = 0x03;
    handle.set_timeout(pipe, 100);
    size_t transferred = 0xFFFF;

    SECTION("can write one small packet")
    {
        uint8_t buffer[32] = { 0x92, 0x44 };
        handle.write_pipe(pipe, buffer, sizeof(buffer), &transferred);
        REQUIRE(transferred == sizeof(buffer));

        // Read the data back.
        uint8_t buffer2[1];
        handle.control_transfer(0xC0, 0x91, 0, 1, buffer2, 1, &transferred);
        REQUIRE(transferred == 1);
        REQUIRE(buffer2[0] == 0x44);
    }

    SECTION("can write one packet with null transferred pointer")
    {
        uint8_t buffer[32] = { 0x92, 0x55 };
        handle.write_pipe(pipe, buffer, sizeof(buffer), NULL);

        // Read the data back.
        uint8_t buffer2[1];
        handle.control_transfer(0xC0, 0x91, 0, 1, buffer2, 1, &transferred);
        REQUIRE(transferred == 1);
        REQUIRE(buffer2[0] == 0x55);

        // Reading back 0x55 confirms that we sent the packet and confirms
        // we did not send any zero-length packets at the end.
    }

    SECTION("can send zero-length trasnfers")
    {
        handle.write_pipe(pipe, NULL, 0, NULL);

        // Expect dataBuffer to contain 0x66
        uint8_t buffer2[1];
        handle.control_transfer(0xC0, 0x91, 0, 1, buffer2, 1, &transferred);
        REQUIRE(transferred == 1);
        REQUIRE(buffer2[0] == 0x66);
    }

    SECTION("can time out")
    {
      // First packet causes a long delay, so this transfer will timeout after
      // a partial data transfer.  Need three packets because of double
      // buffering on the device.
      uint8_t buffer[32 * 3] = { 0xDE, 150, 0 };
      try
      {
        handle.write_pipe(pipe, buffer, sizeof(buffer), &transferred);
        REQUIRE(0);
      }
      catch (const libusbp::error & e)
      {
        REQUIRE(e.has_code(LIBUSBP_ERROR_TIMEOUT));
        #if defined(__linux__) || defined(__APPLE__)
        REQUIRE(transferred == 0);  // bad
        #else
        REQUIRE(transferred == 64);
        #endif
      }
    }
}
#endif
