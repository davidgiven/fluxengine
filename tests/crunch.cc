#include "globals.h"
#include "bytes.h"
#include "common/crunch.h"

static uint8_t outputbuffer[64];

static void test_crunch()
{
    crunch_state_t cs = {};
    Bytes inputdata = { 0x01, 0x7f, 0x80, 0x81, 0x01 };
    cs.inputptr = inputdata.begin();
    cs.inputlen = inputdata.size();
    cs.outputptr = outputbuffer;
    cs.outputlen = 64;
    crunch(&cs);
    donecrunch(&cs);
    Bytes outputdata(outputbuffer, cs.outputptr - outputbuffer);

    assert((outputdata == Bytes{ 0x01, 0x7f, 0xb0, 0x10 }));
}

static void test_uncrunch()
{
    crunch_state_t cs = {};
    Bytes inputdata = { 0x01, 0x7f, 0xb0, 0x10 };
    cs.inputptr = inputdata.begin();
    cs.inputlen = inputdata.size();
    cs.outputptr = outputbuffer;
    cs.outputlen = 64;
    uncrunch(&cs);
    doneuncrunch(&cs);
    Bytes outputdata(outputbuffer, cs.outputptr - outputbuffer);

    assert((outputdata == Bytes { 0x01, 0x7f, 0x80, 0x81, 0x01 }));
}

int main(int argc, const char* argv[])
{
    test_crunch();
    test_uncrunch();
    return 0;
}

