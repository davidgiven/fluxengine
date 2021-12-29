#include <test_helper.h>

// Note: A lot of these tests make specific assumptions about the timing of
// various operations.  If the tests fail intermittently, some parameters may
// need to be adjusted.

const uint8_t pipe_id = 0x82;

#ifdef USE_TEST_DEVICE_A
static void check_error_for_cancelled_transfer(const libusbp::error & error)
{
    if (!error)
    {
        // The transfer actually completed successfully.
        return;
    }

    if (error.has_code(LIBUSBP_ERROR_CANCELLED))
    {
        // This is expected.
    }
    else
    {
        // Some other error happened that we didn't expect.
        throw error;
    }
}

static void clean_up_async_in_pipe(libusbp::async_in_pipe & pipe)
{
    pipe.cancel_transfers();
    test_timeout timeout(500);
    while(pipe.has_pending_transfers())
    {
        pipe.handle_events();
        libusbp::error transfer_error;
        while(pipe.handle_finished_transfer(NULL, NULL, &transfer_error))
        {
            check_error_for_cancelled_transfer(transfer_error);
        }
        timeout.check();
        sleep_quick();
    }
}

static void clean_up_async_in_pipe_and_expect_a_success(libusbp::async_in_pipe & pipe)
{
    pipe.cancel_transfers();

    test_timeout timeout(500);
    uint32_t success_count = 0;
    while(pipe.has_pending_transfers())
    {
        pipe.handle_events();
        libusbp::error transfer_error;
        uint8_t buffer[64] = {0};
        size_t transferred;
        while(pipe.handle_finished_transfer(buffer, &transferred, &transfer_error))
        {
            check_error_for_cancelled_transfer(transfer_error);
            if (!transfer_error)
            {
                REQUIRE(buffer[4] == 0xAB);
                REQUIRE(transferred == 5);
                success_count++;
            }
        }
        timeout.check();
        sleep_quick();
    }
    REQUIRE(success_count > 0);
}
#endif

TEST_CASE("async_in_pipe traits")
{
    libusbp::async_in_pipe pipe, pipe2;

    SECTION("is not copy-constructible")
    {
        REQUIRE(std::is_copy_constructible<libusbp::async_in_pipe>::value == false);

        // Should not compile:
        // libusbp::async_in_pipe pipe3(pipe);
    }

    SECTION("is not copy-assignable")
    {
        REQUIRE(std::is_copy_assignable<libusbp::async_in_pipe>::value == false);

        // Should not compile:
        // pipe2 = pipe;
    }
}

#ifdef USE_TEST_DEVICE_A

TEST_CASE("async_in_pipe basic properties")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);
    libusbp::async_in_pipe pipe = handle.open_async_in_pipe(pipe_id);

    SECTION("is present")
    {
        REQUIRE(pipe);
    }

    SECTION("is movable")
    {
        libusbp::async_in_pipe pipe2 = std::move(pipe);
        REQUIRE(pipe2);
        REQUIRE_FALSE(pipe);
    }

    SECTION("is move-assignable")
    {
        libusbp::async_in_pipe pipe2;
        pipe2 = std::move(pipe);
        REQUIRE(pipe2);
        REQUIRE_FALSE(pipe);
    }
}

TEST_CASE("null async_in_pipe")
{
    libusbp::async_in_pipe pipe;
    std::string expected_message = "Pipe argument is null.";

    SECTION("is not present")
    {
        CHECK_FALSE(pipe);
    }

    SECTION("cannot allocate transfers")
    {
        try
        {
            pipe.allocate_transfers(4, 5);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == expected_message);
        }
    }

    SECTION("cannot start endless transfers")
    {
        try
        {
            pipe.start_endless_transfers();
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == expected_message);
        }
    }

    SECTION("cannot handle events")
    {
        try
        {
            pipe.handle_events();
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == expected_message);
        }
    }

    SECTION("cannot say if it has pending transfers")
    {
        // Test it this way to make sure the C++ wrapper throws an exception
        // instead of ignoring it.
        try
        {
            pipe.has_pending_transfers();
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == expected_message);
        }

        // Also test it this way so we can check the "result" output parameter
        // in the C API.
        bool result = true;
        libusbp::error error(libusbp_async_in_pipe_has_pending_transfers(NULL, &result));
        REQUIRE(error.message() == expected_message);
        REQUIRE_FALSE(result);
    }

    SECTION("cannot handle a finished transfer")
    {
        uint8_t buffer[] = "hi there";
        size_t transferred = 10;
        libusbp::error error = get_some_error();
        try
        {
            pipe.handle_finished_transfer(buffer, &transferred, &error);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == expected_message);
        }
        CHECK(buffer[0] == 'h');
        CHECK(transferred == 0);
        CHECK_FALSE(error);
    }

    SECTION("cannot cancel all transfers")
    {
        try
        {
            pipe.cancel_transfers();
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == expected_message);
        }
    }
}

TEST_CASE("async_in_pipe parameter validation and state checks")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);
    libusbp::async_in_pipe pipe = handle.open_async_in_pipe(0x82);

    SECTION("allocate_transfers")
    {
        SECTION("cannot be called twice on the same pipe")
        {
            pipe.allocate_transfers(1, 64);
            try
            {
                pipe.allocate_transfers(1, 64);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Transfers were already allocated for this pipe.");
            }
        }

        SECTION("does not allow transfer_count to be 0")
        {
            // Set the size to 0 also so we can test that the count is checked
            // before the size (it's nice if the little details stay consistent
            // over time and across platforms).
            try
            {
                pipe.allocate_transfers(0, 0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Transfer count cannot be zero.");
            }
        }

        SECTION("does not allow transfer_size to be 0")
        {
            try
            {
                pipe.allocate_transfers(64, 0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Transfer size cannot be zero.");
            }
        }

        SECTION("rejects transfer sizes too large for the underlying APIs")
        {
            #ifdef _WIN32
            // On Windows, URB buffer sizes are represented by ULONGs.
            if (SIZE_MAX <= ULONG_MAX) { return; }
            size_t too_large_size = (size_t)ULONG_MAX + 1;
            #endif

            #ifdef __linux__
            // On Linux, URB buffer sizes are represented by ints.
            if (SIZE_MAX <= INT_MAX) { return; }
            size_t too_large_size = (size_t)INT_MAX + 1;
            #endif

            #ifdef __APPLE__
            size_t too_large_size = (size_t)UINT32_MAX + 1;
            #endif

            try
            {
                pipe.allocate_transfers(1, too_large_size);
                REQUIRE(0);
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() ==
                    "Failed to allocate transfers for asynchronous IN pipe.  "
                    "Transfer size is too large.");
            }
        }
    }

    SECTION("start_endless_transfers")
    {
        SECTION("complains if transfers were not allocated")
        {
            try
            {
                pipe.start_endless_transfers();
            }
            catch(const libusbp::error & error)
            {
                REQUIRE(error.message() == "Pipe transfers have not been allocated yet.");
            }
        }
    }

    SECTION("has_pending_transfers")
    {
        SECTION("works even if transfers were not allocated")
        {
            REQUIRE_FALSE(pipe.has_pending_transfers());
        }

        SECTION("complains if the output pointer is NULL")
        {
            libusbp::error error(libusbp_async_in_pipe_has_pending_transfers(
                    pipe.pointer_get(), NULL));
            REQUIRE(error.message() == "Boolean output pointer is null.");
        }
    }
}

TEST_CASE("async_in_pipe for an interrupt endpoint")
{
    // If it works for an IN endpoint, it should work for a bulk endpoint too
    // because the underlying APIs that libusbp uses allow us to treat those
    // types of endpoints the same.

    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);
    libusbp::async_in_pipe pipe = handle.open_async_in_pipe(0x82);
    const size_t transfer_size = 5;

    test_timeout timeout(500);

    SECTION("can do continuous transfers and then cancel them")
    {
        pipe.allocate_transfers(5, transfer_size);
        pipe.start_endless_transfers();

        size_t finish_count = 0;
        while(finish_count < 12)
        {
            // Don't use REQUIRE or CATCH here because then the number of
            // assertions printed at the end of the tests will be unpredictable.
            if (!pipe.has_pending_transfers()) { throw "No pending transfers."; }

            uint8_t buffer[transfer_size] = {0};
            size_t transferred;
            libusbp::error transfer_error;
            if (pipe.handle_finished_transfer(buffer, &transferred, &transfer_error))
            {
                if (transfer_error) { throw transfer_error; }

                REQUIRE(buffer[4] == 0xAB);
                finish_count++;
            }

            pipe.handle_events();
            timeout.check();
            sleep_quick();
        }

        REQUIRE(pipe.has_pending_transfers());

        clean_up_async_in_pipe(pipe);
    }

    SECTION("cancelling a transfer before it is completed")
    {
        // Event order tested here: submit, cancel, "reap"

        // Make a lot of transfers and cancel then immediately to make sure at
        // least one was cancelled before it was completed.

        #ifdef __linux__

        // On a normal Linux machine it takes about 1 ms per transfer to cancel
        // transfers.  (Maybe that is because we are using an endpoint with a 1
        // ms polling interval.)  On a VirtualBox machine running on a Windows
        // guest, it takes about much longer: 20 ms per transfer.  To work
        // around this slowness, we only allocate 20 transfers at a time in
        // Linux.

        // Maybe later we should we provide a workaround that lets
        // people close the generic_handle and all its pipes at the same time,
        // thus saving their users from this painful wait, without causing
        // memory leaks.  If the generic handle fd gets closed and replaced with
        // -1, then it would be safe to just free all the resources for the URBs
        // without actually cancelling them.

        // We would like to make the transfer count larger (500) to make this
        // test more resilient to lag caused by the operating system, but that
        // slows down the development process too much.
        const size_t transfer_count = 20;

        #else

        const size_t transfer_count = 500;

        #endif

        pipe.allocate_transfers(transfer_count, transfer_size);
        pipe.start_endless_transfers();

        //printf("Cancelling %d transfers\n", (int)transfer_count);
        test_timeout cancel_timer(10000);
        pipe.cancel_transfers();
        //printf("Done cancelling, took %d ms\n", cancel_timer.get_milliseconds());

        size_t cancel_count = 0;
        test_timeout timeout(10000);
        while(pipe.has_pending_transfers())
        {
            pipe.handle_events();
            libusbp::error transfer_error;
            while(pipe.handle_finished_transfer(NULL, NULL, &transfer_error))
            {
                check_error_for_cancelled_transfer(transfer_error);

                if (transfer_error.has_code(LIBUSBP_ERROR_CANCELLED))
                {
                    cancel_count++;
                }
            }
            timeout.check();
            sleep_quick();
        }

        // Make sure at least one error from the cancelled transfers had the
        // right error code.
        //printf("Cancel count: %d\n", (int)cancel_count);
        REQUIRE(cancel_count > 5);
    }

    SECTION("cancelling a transfer after it is completed but before the libusbp knows")
    {
        // Event order tested here: submit, complete, cancel, "reap"

        pipe.allocate_transfers(1, transfer_size);
        pipe.start_endless_transfers();
        sleep_ms(30);
        clean_up_async_in_pipe_and_expect_a_success(pipe);
    }

    SECTION("cancelling a transfer after it is completed and the event is handled")
    {
        // Event order tested here: submit, complete, "reap", cancel

        pipe.allocate_transfers(1, transfer_size);
        pipe.start_endless_transfers();
        sleep_ms(30);
        pipe.handle_events();
        clean_up_async_in_pipe_and_expect_a_success(pipe);
    }

    SECTION("cancelling a partially completed transfer")
    {
        // Note: This test seems to always fail on Windows Vista and Windows 7
        // because WinUSB reports that 0 bytes have been transferred instead of
        // 10.  It might be that older versions of WinUSB or of the USB stack
        // didn't support returning data from a cancelled, partially completed
        // transfer.

        // Assumption: there will be two packets queued up by Test Device A in
        // its ping-pong buffers.  So when we tell it to pause the ADC for 100
        // ms, a three-packet transfer will quickly receive those two packets
        // and then keep waiting for more.

        // Pause the ADC for 100 ms.
        handle.control_transfer(0x40, 0xA0, 100, 0);

        pipe.allocate_transfers(1, transfer_size * 3);
        pipe.start_endless_transfers();
        sleep_ms(20);

        pipe.cancel_transfers();
        test_timeout timeout(500);
        while(pipe.has_pending_transfers())
        {
            pipe.handle_events();
            libusbp::error transfer_error;
            uint8_t buffer[transfer_size * 3];
            size_t transferred;
            while(pipe.handle_finished_transfer(buffer, &transferred, &transfer_error))
            {
                CHECK(transfer_error);
                check_error_for_cancelled_transfer(transfer_error);

                #if defined(VBOX_LINUX_ON_WINDOWS)
                CHECK(transferred == 0);
                #elif defined(__APPLE__)
                CHECK(transferred == transfer_size);
                CHECK(buffer[4] == 0xAB);
                #else
                CHECK(transferred == transfer_size * 2);
                CHECK(buffer[4] == 0xAB);
                CHECK(buffer[9] == 0xAB);
                #endif
            }
            timeout.check();
            sleep_quick();
        }

        // Unpause the ADC.
        handle.control_transfer(0x40, 0xA0, 0, 0);
    }

    SECTION("cancelling transfers twice is okay")
    {
        pipe.allocate_transfers(1, transfer_size);
        pipe.start_endless_transfers();
        pipe.cancel_transfers();
        clean_up_async_in_pipe(pipe);
    }

    SECTION("might respect the time out from set_timeout")
    {
        #ifdef __linux__
        // On Linux, asynchronous requests cannot have a timeout, so this test
        // will not pass.  If we want to implement a timeout feature, we would
        // need to make some kind of timer like libusb does, and cancel the
        // requests when the time is up.  It's probably fine for the user to
        // just implement a timeout themselves using a separate time library.
        return;
        #endif

        #ifdef __APPLE__
        // On Mac OS X, interrupt endpoints cannot have a timeout, and our
        // library simply doesn't use a timeout (we use ReadPipeAsync instead of
        // ReadPipeAsyncTO).
        return;
        #endif

        // Pause the ADC for 200 ms.
        handle.control_transfer(0x40, 0xA0, 200, 0);

        handle.set_timeout(pipe_id, 10);
        pipe.allocate_transfers(5, transfer_size);
        pipe.start_endless_transfers();

        uint32_t timeout_count = 0;
        uint32_t success_count = 0;
        test_timeout timeout(500);
        while(timeout_count < 8)
        {
            pipe.handle_events();
            libusbp::error transfer_error;
            while(pipe.handle_finished_transfer(NULL, NULL, &transfer_error))
            {
                // We expect there to be 0 to 2 successes, and the rest of the
                // transfers will be timeouts.
                if (transfer_error == false)
                {
                    // Successful transfer.
                    success_count++;

                    if (timeout_count != 0)
                    {
                        throw "Got a success after a timeout.";
                    }
                    if (success_count > 2)
                    {
                        throw "Got too many successes.";
                    }
                }
                else if (transfer_error.has_code(LIBUSBP_ERROR_TIMEOUT))
                {
                    // Timeout.

                    const char * expected =
                        "Asynchronous IN transfer failed.  "
                        "The operation timed out.  "
                        #ifdef _WIN32
                        "Windows error code 0x79."
                        #endif
                        ;
                    REQUIRE(transfer_error.message() == expected);
                    timeout_count++;
                }
                else
                {
                    // Unexpected error.
                    throw transfer_error;
                }
            }
            timeout.check();
            sleep_quick();
        }

        // Clean up the pipe, while accepting that there might be more transfers
        // finishing with a cancellation error.
        pipe.cancel_transfers();
        test_timeout timeout_cleanup(500);
        while(pipe.has_pending_transfers())
        {
            pipe.handle_events();
            libusbp::error transfer_error;
            while(pipe.handle_finished_transfer(NULL, NULL, &transfer_error))
            {
                if (transfer_error
                    && !transfer_error.has_code(LIBUSBP_ERROR_CANCELLED)
                    && !transfer_error.has_code(LIBUSBP_ERROR_TIMEOUT))
                {
                    throw transfer_error;
                }
            }
            timeout_cleanup.check();
            sleep_quick();
        }

        // Unpause the ADC.
        handle.control_transfer(0x40, 0xA0, 0, 0);
    }

    SECTION("overflow")
    {
        #ifdef VBOX_LINUX_ON_WINDOWS
        // This test fails and then puts the USB device into a weird state
        // if run on Linux inside VirtualBox on a Windows host.
        std::cerr << "Skipping asynchronous IN pipe overflow test.\n";
        return;
        #endif

        pipe.allocate_transfers(1, transfer_size + 1);
        pipe.start_endless_transfers();
        sleep_ms(10);
        pipe.handle_events();

	uint8_t buffer[transfer_size] = {0};
	size_t transferred;
	libusbp::error transfer_error;
	bool finished = pipe.handle_finished_transfer(buffer,
            &transferred, &transfer_error);
        REQUIRE(finished);

        std::string expected_message;
        size_t expected_transferred;
        #ifdef _WIN32
        // This request is an error in WinUSB since we are using RAW_IO.  The
        // error is detected before any data is transferred.
        expected_transferred = 0;
        expected_message = "Asynchronous IN transfer failed.  "
            "Incorrect function.  Windows error code 0x1.";
        #elif defined(__linux__)
        // This request results in an error in Linux but it is only detected
        // after some data is transferred.
        expected_transferred = transfer_size + 1;
        expected_message = "Asynchronous IN transfer failed.  "
            "The transfer overflowed.  Error code 75.";
        #elif defined(__APPLE__)
        // On Mac OS X, this results in an error after some data is transferred.
        expected_transferred = transfer_size;
        expected_message = "Asynchronous IN transfer failed.  "
            "The transfer overflowed.  Error code 0xe00002e8.";
        #else
        REQUIRE(0);
        #endif

        CHECK(transferred == expected_transferred);
        CHECK(transfer_error.message() == expected_message);

        // We can't just call clean_up_async_in_pipe because we might be running
        // on a slower computer and the next transfer has actually already been
        // queued.
        pipe.cancel_transfers();
        test_timeout timeout(500);
        while(pipe.has_pending_transfers())
        {
            pipe.handle_events();
            libusbp::error transfer_error;
            while(pipe.handle_finished_transfer(NULL, NULL, &transfer_error))
            {
                if (!transfer_error)
                {
                    #if defined(__linux__)
                    // Overflowing is always an error, none of the transfers
                    // should have succeeded.
                    throw "Expected to get a transfer error.";
                    #endif
                }
                else if (transfer_error.message() == expected_message)
                {
                    // This is fine; the error indicates an overflow.
                }
                #ifdef __APPLE__
                else if (transfer_error.has_code(LIBUSBP_ERROR_STALL))
                {
                    // Mac OS X considers the pipe to be "stalled" after an
                    // overflow happens and we cannot actually submit any more
                    // transfers.
                }
                #endif
                else if (transfer_error.has_code(LIBUSBP_ERROR_CANCELLED))
                {
                    // This is fine; the transfer was cancelled.
                }
                else
                {
                    // Unexpected error.
                    throw transfer_error;
                }
            }
            timeout.check();
            sleep_quick();
        }

        clean_up_async_in_pipe(pipe);
    }

    #ifdef __linux__
    SECTION("does not prevent the creation of another generic_interface object")
    {
        // It seems that the "usbfs" driver gets attached to the device
        // while we are doing asynchronous transfers.  This tests that our
        // code is OK with that and we can still open up other handles
        // to the device, at least in Linux.

        pipe.allocate_transfers(1, transfer_size);
        pipe.start_endless_transfers();
        libusbp::generic_interface gi2(device, 0, true);
        libusbp::generic_interface gi3(device, 0, true);
        libusbp::generic_handle handle2(gi3);
        handle2.control_transfer(0x40, 0x90, 1, 0);
        clean_up_async_in_pipe(pipe);
    }
    #endif
}

#endif
