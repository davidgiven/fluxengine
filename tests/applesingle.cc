#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/vfs/applesingle.h"
#include "snowhouse/snowhouse.h"

using namespace snowhouse;

static void testOverhead()
{
    AppleSingle t;
    t.data = Bytes("This is the data fork");
    t.rsrc = Bytes("This is the resource fork");

    AssertThat(t.render().size(),
        Equals(t.data.size() + t.rsrc.size() + AppleSingle::OVERHEAD));
}

static void testRoundtrip()
{
    AppleSingle t;
    t.data = Bytes("This is the data fork");
    t.rsrc = Bytes("This is the resource fork");
    t.creator = Bytes("CREA");
    t.type = Bytes("TYPE");

    Bytes tb = t.render();
    AppleSingle s;
    s.parse(tb);

    AssertThat(s.data, Equals(t.data));
    AssertThat(s.rsrc, Equals(t.rsrc));
    AssertThat(s.creator, Equals(t.creator));
    AssertThat(s.type, Equals(t.type));
}

int main(void)
{
    testOverhead();
    testRoundtrip();
    return 0;
}
