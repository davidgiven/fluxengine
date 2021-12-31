#include <libusbp.hpp>
#include <catch.hpp>
#include <iostream>
#include <chrono>
#include <type_traits>
#include <unistd.h>

extern "C"
{
  #include <libusbp_internal.h>
}

libusbp::device find_by_vendor_id_product_id(uint16_t vendor_id, uint16_t product_id);
libusbp::device find_test_device_a();
libusbp::device find_test_device_b();

class test_timeout
{
    // monotonic_clock was renamed to steady_clock, but GCC 4.6 only defines
    // monotonic_clock, and later versions only define steady_clock.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6
    typedef std::chrono::monotonic_clock clock;
#else
    typedef std::chrono::steady_clock clock;
#endif

public:
    test_timeout(uint32_t timeout_ms)
    {
        start = clock::now();
        this->timeout_ms = timeout_ms;
    }

    void check()
    {
        if (get_milliseconds() > timeout_ms)
        {
            throw "Timeout";
        }
    }

    uint32_t get_milliseconds()
    {
        clock::time_point now = clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    }

private:
    uint32_t timeout_ms;
    clock::time_point start;
};

#ifdef _WIN32
void inline sleep_quick()
{
    Sleep(1);
}

void inline sleep_ms(uint32_t ms)
{
    Sleep(ms);
}
#else
void inline sleep_quick()
{
    usleep(100);
}

void inline sleep_ms(uint32_t ms)
{
    usleep((useconds_t)1000 * ms);
}
#endif

inline libusbp::error get_some_error()
{
    return libusbp::error(libusbp_device_copy(NULL, NULL));
}
