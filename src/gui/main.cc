#include "globals.h"
#include "ui.h"
#include "uipp.h"
#include "threads.h"
#include "gui.h"
#include "fmt/format.h"
#include <unistd.h>

static int quit_cb(void* data)
{
	return 1;
}

int main(int argc, const char* argv[])
{
	uiInitOptions o = {0};
	uiInit(&o);
	UIInitThreading();

	uiMenu* menu = uiNewMenu("File");
	uiMenuItem* item = uiMenuAppendQuitItem(menu);
	uiOnShouldQuit(quit_cb, NULL);

	auto app = createMainApp();
	app->show();

	uiMain();
	return 0;
}

