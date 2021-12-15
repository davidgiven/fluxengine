#include <test_helper.h>

#if defined(__linux__) && !defined(NDEBUG)

TEST_CASE("usbfd_check_existence")
{
    SECTION("gives a nice error if the file does not exist")
    {
        libusbp::error error(usbfd_check_existence("doesnt-exist-%%%"));
        REQUIRE(error.has_code(LIBUSBP_ERROR_NOT_READY));
    }

    SECTION("gives a nice error if part of the file path does not exist")
    {
        libusbp::error error(usbfd_check_existence("doesnt-exist-%%%/xx"));
        REQUIRE(error.has_code(LIBUSBP_ERROR_NOT_READY));
    }
}

#endif
