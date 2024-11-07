#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "protocol.h"
#include "fmt/format.h"
#include "tests.h"
#include <sstream>

static Fluxmap fluxmap(Bytes{F_DESYNC,
    F_BIT_PULSE | 0x30,
    F_BIT_INDEX | 0x30,
    F_BIT_PULSE | F_BIT_INDEX | 0x30,
    0x30,
    F_BIT_PULSE | 0x30,
    F_DESYNC,
    F_BIT_PULSE | 0x30,
    0x30,
    F_DESYNC,
    0x30,
    F_BIT_PULSE | 0x30});

#define ASSERT_NEXT_EVENT(wantedEvent, wantedTicks)                         \
    do                                                                      \
    {                                                                       \
        unsigned gotTicks;                                                  \
        int gotEvent;                                                       \
        fmr.getNextEvent(gotEvent, gotTicks);                               \
        assert((gotEvent == (wantedEvent)) && (gotTicks == (wantedTicks))); \
    } while (0)

#define ASSERT_READ_SPECIFIC_EVENT(wantedEvent, wantedTicks) \
    do                                                       \
    {                                                        \
        unsigned gotTicks;                                   \
        fmr.findEvent(wantedEvent, gotTicks);                \
        assertThat(gotTicks).isEqualTo(wantedTicks);         \
    } while (0)

void test_read_all_events()
{
    FluxmapReader fmr(fluxmap);
    ASSERT_NEXT_EVENT(F_DESYNC, 0);
    ASSERT_NEXT_EVENT(F_BIT_PULSE, 0x30);
    ASSERT_NEXT_EVENT(F_BIT_INDEX, 0x30);
    ASSERT_NEXT_EVENT(F_BIT_PULSE | F_BIT_INDEX, 0x30);
    ASSERT_NEXT_EVENT(F_BIT_PULSE, 0x60);
    ASSERT_NEXT_EVENT(F_DESYNC, 0);
    ASSERT_NEXT_EVENT(F_BIT_PULSE, 0x30);
    ASSERT_NEXT_EVENT(F_DESYNC, 0x30);
    ASSERT_NEXT_EVENT(F_BIT_PULSE, 0x60);
    ASSERT_NEXT_EVENT(F_EOF, 0);
    ASSERT_NEXT_EVENT(F_EOF, 0);
}

void test_read_pulses()
{
    FluxmapReader fmr(fluxmap);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_PULSE, 0x30);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_PULSE, 0x60);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_PULSE, 0x60);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_PULSE, 0x30);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_PULSE, 0x90);
}

void test_read_indices()
{
    FluxmapReader fmr(fluxmap);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_INDEX, 0x60);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_INDEX, 0x30);
    ASSERT_READ_SPECIFIC_EVENT(F_BIT_INDEX, 6 * 0x30);
}

void test_read_desyncs()
{
    FluxmapReader fmr(fluxmap);
    ASSERT_READ_SPECIFIC_EVENT(F_DESYNC, 0);
    ASSERT_READ_SPECIFIC_EVENT(F_DESYNC, 0xf0);
    ASSERT_READ_SPECIFIC_EVENT(F_DESYNC, 0x60);
    ASSERT_READ_SPECIFIC_EVENT(F_DESYNC, 0x60);
}

int main(int argc, const char* argv[])
{
    test_read_all_events();
    test_read_pulses();
    test_read_indices();
    test_read_desyncs();
    return 0;
}
