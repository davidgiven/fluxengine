#include "globals.h"
#include "ui.h"
#include "uipp.h"
#include "uippmenu.h"
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

	auto menu = UIMenu("File");
	menu.add("Test menu item!")->disable();
	menu.addQuit();

	uiOnShouldQuit(quit_cb, NULL);

	auto app = createMainApp();
	app->show();

	uiMain();
	return 0;
}

