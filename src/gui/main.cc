#include "globals.h"
#include "ui.h"

static uiWindow* mainwin;

int main(int argc, const char* argv[])
{
	uiInitOptions o = {0};
	uiInit(&o);

	mainwin = uiNewWindow("example", 640, 480, 1);
	uiControlShow(uiControl(mainwin));
	uiMain();
	return 0;
}

