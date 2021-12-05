#include "globals.h"
#include "ui.h"

static uiWindow* mainwin;
static uiArea* area;

static int close_cb(uiWindow* window, void* data)
{
	uiQuit();
	return 1;
}

static int quit_cb(void* data)
{
	return 1;
}

static void handlerDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *p)
{
}

static void handlerMouseEvent(uiAreaHandler *a, uiArea *area, uiAreaMouseEvent *
e)
{
}

static void handlerMouseCrossed(uiAreaHandler *ah, uiArea *a, int left)
{
        // do nothing
}

static void handlerDragBroken(uiAreaHandler *ah, uiArea *a)
{
        // do nothing
}

static int handlerKeyEvent(uiAreaHandler *ah, uiArea *a, uiAreaKeyEvent *e)
{
        // reject all keys
        return 0;
}

static const uiAreaHandler handler = {
	handlerDraw,
	handlerMouseEvent,
	handlerMouseCrossed,
	handlerDragBroken,
	handlerKeyEvent
};


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

	uiBox* hbox = uiNewHorizontalBox();
        uiBoxSetPadded(hbox, 1);
        uiWindowSetChild(mainwin, uiControl(hbox));

	uiControlShow(uiControl(mainwin));

	area = uiNewArea((uiAreaHandler*) &handler);
	uiBoxAppend(hbox, uiControl(area), 1);

	uiMain();
	return 0;
}

