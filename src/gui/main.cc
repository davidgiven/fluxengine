#include "globals.h"
#include "ui.h"
#include "uipp.h"
#include "fmt/format.h"

static uiDrawStrokeParams STROKE = {
	uiDrawLineCapFlat,
	uiDrawLineJoinMiter,
	0.5,
	1.0,
	nullptr,
	0,
	0.0
};

static uiDrawBrush WHITE = { uiDrawBrushTypeSolid, 1.0, 0.0, 1.0, 1.0 };
static uiDrawBrush BLACK = { uiDrawBrushTypeSolid, 0.0, 0.0, 0.0, 1.0 };

static int quit_cb(void* data)
{
	return 1;
}

class MainWindow : public UIWindow
{
public:
	MainWindow():
		UIWindow("FluxEngine", 640, 480)
	{}

	int onClose() override
	{
		uiQuit();
		return 1;
	}
};

class MainArea : public UIArea
{
public:
	void onRedraw(uiAreaDrawParams* p)
	{
		UIPath(p).rectangle(0, 0, p->AreaWidth, p->AreaHeight).fill(WHITE);
		UIPath(p).begin(0, 0).lineTo(p->AreaWidth, p->AreaHeight).end().stroke(BLACK, STROKE);
	}
};

int main(int argc, const char* argv[])
{
	uiInitOptions o = {0};
	uiInit(&o);

	uiMenu* menu = uiNewMenu("File");
	uiMenuItem* item = uiMenuAppendQuitItem(menu);
	uiOnShouldQuit(quit_cb, NULL);

	UIAllocator a;
	
	auto window =
		a.make<MainWindow>()
			->setChild(
				a.make<UIVBox>()
					->add(a.make<UIButton>("press me!")->build())
					->add(a.make<MainArea>()->setStretchy(true)->build())
					->add(a.make<UIButton>("press me again!")->build())
					->build())
			->build();
	uiOnShouldQuit(quit_cb, NULL);

	window->show();
	uiMain();
	return 0;
}

