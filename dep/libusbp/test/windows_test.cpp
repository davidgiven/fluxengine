#include <test_helper.h>

#ifdef _WIN32

TEST_CASE("CM_Locate_DevNode", "[cmld]")
{
    CONFIGRET cr;
    DEVINST dev_inst;

    SECTION("totally invalid id")
    {
        char id[] = "totally invalid";
        cr = CM_Locate_DevNode(&dev_inst, id, CM_LOCATE_DEVNODE_NORMAL);
        REQUIRE(cr == CR_INVALID_DEVICE_ID);
    }

    SECTION("non-existent ID")
    {
        char id[] = "USB\\VID_1FFB&PID_0001\\01eeca1";
        cr = CM_Locate_DevNode(&dev_inst, id, CM_LOCATE_DEVNODE_NORMAL);
        REQUIRE(cr == CR_NO_SUCH_DEVNODE);
    }

    SECTION("unplugged USB device")
    {
        // To make this test do something, set the id below to the
        // device instance ID of a USB device that is currently
        // unplugged from your computer, but was plugged in at one
        // point.  Also, uncomment the early return.

        char id[] = "USB\\VID_1FFB&PID_0100\\6E-D7-65-51";
        return;

        cr = CM_Locate_DevNode(&dev_inst, id, CM_LOCATE_DEVNODE_NORMAL);
        CHECK(cr == CR_NO_SUCH_DEVNODE);
        cr = CM_Locate_DevNode(&dev_inst, id, CM_LOCATE_DEVNODE_PHANTOM);
        CHECK(cr == CR_SUCCESS);
    }
}

#endif
