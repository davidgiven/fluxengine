#include "globals.h"
#include "ui.h"

static uiWindow* mainwin;

static int close_cb(uiWindow* window, void* data)
{
	uiQuit();
	return 1;
}

static int quit_cb(void* data)
{
	return 1;
}

int main(int argc, const char* argv[])
{
	uiInitOptions o = {0};
	uiInit(&o);

	uiMenu* menu = uiNewMenu("File");
	uiMenuItem* item = uiMenuAppendQuitItem(menu);
	uiOnShouldQuit(quit_cb, NULL);

	mainwin = uiNewWindow("example", 640, 480, 1);
	uiWindowOnClosing(mainwin, close_cb, NULL);
	uiOnShouldQuit(quit_cb, NULL);

	uiControlShow(uiControl(mainwin));
	uiMain();
	return 0;
}

