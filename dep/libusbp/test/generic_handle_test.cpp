#include <test_helper.h>

TEST_CASE("generic_handle traits")
{
    libusbp::generic_handle handle, handle2;

    SECTION("is not copy-constructible")
    {
        REQUIRE_FALSE(std::is_copy_constructible<libusbp::generic_handle>::value);

        // Should not compile:
        // libusbp::generic_handle handle3(handle);
    }

    SECTION("is not copy-assignable")
    {
        REQUIRE_FALSE(std::is_copy_assignable<libusbp::generic_handle>::value);

        // Should not compile:
        // handle2 = handle;
    }
}

TEST_CASE("generic handles cannot be created from a NULL generic interface")
{
    try
    {
        libusbp::generic_interface gi;
        libusbp::generic_handle handle(gi);
        REQUIRE(0);
    }
    catch(const libusbp::error & error)
    {
        REQUIRE(std::string(error.what()) == "Generic interface is null.");
    }
}

TEST_CASE("null generic_handle")
{
    libusbp::generic_handle handle;

    SECTION("is not present")
    {
        REQUIRE_FALSE(handle);
    }

    SECTION("can be closed (but it makes no difference)")
    {
        handle.close();
        REQUIRE_FALSE(handle);
    }

    SECTION("cannot open async pipes")
    {
        try
        {
            handle.open_async_in_pipe(0x82);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == "Generic handle is null.");
        }
    }

    SECTION("cannot set timeouts")
    {
        try
        {
            handle.set_timeout(0, 0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == "Generic handle is null.");
        }
    }

    SECTION("cannot do control transfers")
    {
        try
        {
            handle.control_transfer(0x40, 0x90, 0, 0);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == "Generic handle is null.");
        }
    }

    SECTION("cannot read a pipe")
    {
        try
        {
            handle.read_pipe(0x82, NULL, 0, NULL);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == "Generic handle is null.");
        }
    }

    SECTION("cannot open an asynchronous IN pipe")
    {
        try
        {
            handle.open_async_in_pipe(0x82);
        }
        catch(const libusbp::error & error)
        {
            REQUIRE(error.message() == "Generic handle is null.");
        }
    }

    SECTION("exports invalid underlying handles")
    {
#if defined(_WIN32)
        REQUIRE(handle.get_winusb_handle() == INVALID_HANDLE_VALUE);
#elif defined(__linux__)
        REQUIRE(handle.get_fd() == -1);
#elif defined(__APPLE__)
        REQUIRE(handle.get_cf_plug_in() == NULL);
#else
        REQUIRE(0);
#endif
    }
}

TEST_CASE("generic handle parameter validation and corner cases")
{
    SECTION("libusbp_generic_handle_open")
    {
        SECTION("complains if the output pointer is null")
        {
            libusbp::error error(libusbp_generic_handle_open(NULL, NULL));
            REQUIRE(error.message() == "Generic handle output pointer is null.");
        }

        SECTION("sets the output to NULL is possible")
        {
            libusbp_generic_handle * p = (libusbp_generic_handle *)1;
            libusbp::error error(libusbp_generic_handle_open(NULL, &p));
            REQUIRE((p == NULL));
        }
    }

    SECTION("libusbp_generic_handle_open_async_in_pipe")
    {
        SECTION("complains if the output pointer is null")
        {
            libusbp::error error(libusbp_generic_handle_open_async_in_pipe(NULL, 0, NULL));
            REQUIRE(error.message() == "Pipe output pointer is null.");
        }

        SECTION("sets the output pointer to NULL if possible")
        {
            libusbp_async_in_pipe * p = (libusbp_async_in_pipe *)1;
            libusbp::error error(libusbp_generic_handle_open_async_in_pipe(NULL, 0, &p));
            REQUIRE((p == NULL));
        }
    }
}

#ifdef USE_TEST_DEVICE_A
TEST_CASE("generic_handle instance", "[ghitda]")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);
    libusbp::generic_handle handle(gi);

    SECTION("is present")
    {
        REQUIRE(handle);
    }

    SECTION("is movable")
    {
        libusbp::generic_handle handle2 = std::move(handle);
        REQUIRE(handle2);
        REQUIRE_FALSE(handle);
    }

    SECTION("is move-assignable")
    {
        libusbp::generic_handle handle2;
        handle2 = std::move(handle);
        REQUIRE(handle2);
        REQUIRE_FALSE(handle);
    }

    SECTION("can be closed")
    {
        handle.close();
        REQUIRE_FALSE(handle);
    }

    SECTION("can export its underlying handles")
    {
#if defined(_WIN32)
        REQUIRE((handle.get_winusb_handle() != NULL));
        REQUIRE((handle.get_winusb_handle() != INVALID_HANDLE_VALUE));
#elif defined(__linux__)
        REQUIRE(handle.get_fd() > 2);
#elif defined(__APPLE__)
        IOCFPlugInInterface ** plug_in = (IOCFPlugInInterface **)handle.get_cf_plug_in();
        REQUIRE(plug_in != NULL);
        IOUSBInterfaceInterface197 ** ioh;
        HRESULT hr = (*plug_in)->QueryInterface(plug_in,
            CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID197), (void **)&ioh);
        REQUIRE(!hr);
        (*ioh)->Release(ioh);
#else
        REQUIRE(0);  // this platform is missing a way to export its underlying handles
#endif
    }
}

TEST_CASE("generic_handle creation for Test Device A")
{
    libusbp::device device = find_test_device_a();
    libusbp::generic_interface gi(device, 0, true);

    SECTION("can be created and closed quickly several times")
    {
        for(unsigned int i = 0; i < 10; i++)
        {
            libusbp::generic_handle handle(gi);
        }
    }

#if defined(_WIN32) || defined(__APPLE__)
    SECTION("only one per interface can be open at a time")
    {
        libusbp::generic_handle handle1(gi);
        try
        {
            libusbp::generic_handle handle2(gi);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            CHECK(error.has_code(LIBUSBP_ERROR_ACCESS_DENIED));
        }
    }
#else
    SECTION("multiple handles to the same interface can be open at one time")
    {
        libusbp::generic_handle handle1(gi);
        libusbp::generic_handle handle2(gi);
    }
#endif
}
#endif

#ifdef USE_TEST_DEVICE_B
TEST_CASE("generic_handle creation for Test Device B")
{
    libusbp::device device = find_test_device_b();
    libusbp::generic_interface gi(device, 0, false);

    SECTION("can be created and closed quickly several times")
    {
        for(unsigned int i = 0; i < 10; i++)
        {
            libusbp::generic_handle handle(gi);
        }
    }

#if defined(_WIN32) || defined(__APPLE__)
    SECTION("only one per interface can be open at a time")
    {
        libusbp::generic_handle handle1(gi);
        try
        {
            libusbp::generic_handle handle2(gi);
            REQUIRE(0);
        }
        catch(const libusbp::error & error)
        {
            CHECK(error.has_code(LIBUSBP_ERROR_ACCESS_DENIED));
        }
    }
#else
    SECTION("multiple handles to the same interface can be open at one time")
    {
        libusbp::generic_handle handle1(gi);
        libusbp::generic_handle handle2(gi);
    }
#endif
}
#endif
