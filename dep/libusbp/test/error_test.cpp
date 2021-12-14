/* Tests the generic code for dealing with error objects.
 * These tests don't really have anything to do with USB. */

#include <test_helper.h>

#ifndef NDEBUG

TEST_CASE("error class basic properties")
{
    SECTION("can be caught as a const std::exception &")
    {
        try
        {
            throw libusbp::error();
            REQUIRE(0);
        }
        catch(const std::exception & e)
        {
            REQUIRE(1);
            CHECK(std::string(e.what()) == "No error.");
        }
    }

    SECTION("can be moved without getting copied")
    {
        // This isn't really necessary for a light-weight error object, but it
        // is a feature that unique_pointer_wrapper_with_copy is supposed to
        // provide, so we want to test it on at least one of the classes that
        // uses unique_pointer_wrapper.

        libusbp::error error(error_create("hi"));
        libusbp_error * p = error.pointer_get();
        REQUIRE(p);

        // Move constructor.
        libusbp::error error2 = std::move(error);
        REQUIRE(error2.pointer_get() == p);
        REQUIRE_FALSE(error);

        // Move assignment.
        error = std::move(error2);
        REQUIRE(error.pointer_get() == p);
        REQUIRE_FALSE(error2);
    }
}


TEST_CASE("null error", "[null_error]")
{
    libusbp::error error;

    SECTION("can also be constructed by passing in NULL")
    {
        libusbp::error error(NULL);
    }

    SECTION("has a default message")
    {
        REQUIRE(error.message() == "No error.");
    }

    SECTION("is not present")
    {
        REQUIRE_FALSE(error);
    }

    SECTION("can be copied")
    {
        libusbp::error error2 = error;
        REQUIRE_FALSE(error2);
    }

    SECTION("has no error codes, and you can not add codes to it")
    {
        REQUIRE_FALSE(error.has_code(1));
    }
}

TEST_CASE("error_create", "[error_create]")
{
    SECTION("creates a non-null error")
    {
        libusbp::error error(error_create("Error1 %d.", 123));
        REQUIRE(error);
    }

    SECTION("properly formats its input")
    {
        libusbp::error error(error_create("Error1 %d.", 123));
        REQUIRE(error.message() == "Error1 123.");
    }
}

TEST_CASE("error_add", "[error_add]")
{
    SECTION("works with NULL")
    {
        libusbp::error error(error_add(NULL, "hi"));
        REQUIRE(error.message() == "hi");
    }

    SECTION("preserves the message and error codes of errors passed to it")
    {
        libusbp::error error(error_add(error_add_code(error_create("hi1"), 7), "hi2"));
        CHECK(error.message() == "hi2  hi1");
        CHECK(error.has_code(7));
    }
}

TEST_CASE("error_add_code", "[error_add_code]")
{
    SECTION("works with NULL")
    {
        libusbp::error error(error_add_code(NULL, 4));
        CHECK(error.message() == "");
        CHECK(error.has_code(4));
    }

    SECTION("preserves the message and error codes of errors passed to it")
    {
        libusbp::error error(error_add_code(error_add_code(error_create("hi1"), 7), 9));
        CHECK(error.message() == "hi1");
        CHECK(error.has_code(7));
        CHECK(error.has_code(9));
    }
}

TEST_CASE("error_no_memory")
{
    libusbp::error error(&error_no_memory);

    SECTION("has the right message")
    {
        REQUIRE(error.message() == "Failed to allocate memory.");
    }

    SECTION("has the right code")
    {
        REQUIRE(error.has_code(LIBUSBP_ERROR_MEMORY));
    }
}

TEST_CASE("libusbp_error_copy")
{
    libusbp_error * error = error_create("X.");
    error = error_add(error, "Y.");
    error = error_add_code(error, 7);
    error = error_add_code(error, 8);
    libusbp::error error1(error);

    // Test the C++ copy constructor and libusbp_error_copy at the same time.
    libusbp::error error2 = error1;

    SECTION("copies the message")
    {
        REQUIRE(error2.message() == "Y.  X.");
    }

    SECTION("copies codes")
    {
        CHECK(error2.has_code(7));
        CHECK(error2.has_code(8));
    }
}

#endif
