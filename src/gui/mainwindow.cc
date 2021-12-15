#include "globals.h"
#include "ui.h"
#include "uipp.h"
#include "uipppath.h"
#include "gui.h"
#include "threads.h"
#include "proto.h"
#include <unistd.h>

using namespace std::placeholders;

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

static const UIGridStyle GRID_LEFT = { .halign = uiAlignStart, .valign = uiAlignCenter };
static const UIGridStyle GRID_RIGHT = { .halign = uiAlignEnd, .valign = uiAlignCenter };
static const UIGridStyle GRID_FILLX = { .hexpand = true, .vexpand = false };
static const UIGridStyle GRID_FILLXY = { .hexpand = true, .vexpand = true };
static const UIGridStyle GRID_SPAN1 = { .xspan = 2, .hexpand = true, .vexpand = true };
static const UIGridStyle GRID_CENTRE = { .halign = uiAlignCenter, .valign = uiAlignCenter };
static const UIGridStyle GRID_TOPLEFT = { .halign = uiAlignStart, .valign = uiAlignStart };

class MainApp : public UIAllocator, public Showable
{
public:
	MainApp()
	{
		auto configs = findConfigs();
		_window = make<UIWindow>("FluxEngine", 640, 480)
			->setOnClose(_onclose_cb)
			->setChild(
				make<UITab>()
					->add("Configuration", make<UIForm>()
						->setPadding(2)
						->add("Interface:", false, make<UISelect<std::string>>())
						->add("Drive:", false, make<UICombo>()
							->setOptions({
								"drive:0",
								"drive:1"
							}))
						->add("Format:", false, make<UISelect<ConfigProto*>>()->setOptions(configs))
						->add("", true, make<UIGrid>()
							->add(0, 0, GRID_TOPLEFT, make<UIButton>("Next"))))
					->add("Flux", make<UIVBox>()
						->add(make<UIGrid>()
							->setPadding(2)
							->add(0, 0, GRID_LEFT, make<UILabel>("Cylinder:"))
							->add(1, 0, GRID_LEFT, make<UILabel>("0"))
							->add(2, 0, GRID_FILLX, make<UILabel>(""))
							->add(3, 0, GRID_RIGHT, make<UILabel>("Head:"))
							->add(4, 0, GRID_LEFT, make<UILabel>("0")))
						->add(true, make<UIArea>()
							->setOnDraw(_onredraw_cb)
							->disable())
						->add(true, make<UIArea>()
							->setOnDraw(_onredraw_cb)
							->disable()))
					->add("Sectors", make<UILabel>("Sectors"))
			);

		UIStartAppThread(_appthread_cb, _appthreadexit_cb);
	}

	void show()
	{
		_window->show();
	}

	void hide()
	{
		_window->hide();
	}

private:
	std::function<int()> _onclose_cb =
		[&] {
			uiQuit();
			return 1;
		};

	std::function<void(uiAreaDrawParams*)> _onredraw_cb =
		[&](auto p) {
			UIPath(p).rectangle(0, 0, p->AreaWidth, p->AreaHeight).fill(WHITE);
			UIPath(p).begin(0, 0).lineTo(p->AreaWidth, p->AreaHeight).end().stroke(BLACK, STROKE);
		};

	std::function<void()> _appthread_cb =
		[&] {
			//UIRunOnUIThread([&] { _busyButton->setText("Busy"); });
			//sleep(5);
		};

	std::function<void()> _appthreadexit_cb =
		[&] {
			//_busyButton->setText("Not busy");
		};

private:
	std::vector<std::pair<std::string, ConfigProto*>> findConfigs()
	{
		std::vector<std::pair<std::string, ConfigProto*>> configs;
		for (const auto& it : formats)
		{
			auto& config = _configs.emplace_back();
			if (!config.ParseFromString(it.second))
				Error() << "couldn't load built-in config proto";

			configs.push_back(std::make_pair(config.comment(), &config));
		}
		return configs;
	}

private:
	UIWindow* _window;
	UIButton* _busyButton;
	std::vector<ConfigProto> _configs;
};

std::unique_ptr<Showable> createMainApp()
{
	return std::make_unique<MainApp>();
}

