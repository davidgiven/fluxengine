#ifndef UIPPMENU_H
#define UIPPMENU_H

class UIMenuItem : public Closeable
{
public:
	UIMenuItem(uiMenuItem* item, bool addCallback = true):
		_item(item)
	{
		if (addCallback)
			uiMenuItemOnClicked(_item, onclick_cb, this);
	}

	UIMenuItem* enable()
	{ uiMenuItemEnable(_item); return this; }

	UIMenuItem* disable()
	{ uiMenuItemDisable(_item); return this; }

	UIMenuItem* setOnClick(std::function<void()> cb)
	{ _callback = cb; return this; }

private:
	static void onclick_cb(uiMenuItem*, uiWindow*, void* p)
	{
		auto item = (UIMenuItem*)p;
		if (item->_callback)
			item->_callback();
	}

private:
	uiMenuItem* _item;
	std::function<void()> _callback;
};

class UIMenu : public UIAllocator, public Closeable
{
public:
	UIMenu(const std::string& name):
		_menu(uiNewMenu(name.c_str()))
	{}

	UIMenuItem* add(const std::string& name)
	{ auto item = uiMenuAppendItem(_menu, name.c_str()); return make<UIMenuItem>(item); }

	UIMenuItem* addQuit()
	{ auto item = uiMenuAppendQuitItem(_menu); return make<UIMenuItem>(item, false); }

	UIMenuItem* addPreferences()
	{ auto item = uiMenuAppendPreferencesItem(_menu); return make<UIMenuItem>(item); }

	UIMenuItem* addAbout()
	{ auto item = uiMenuAppendAboutItem(_menu); return make<UIMenuItem>(item); }

	void addSeparator()
	{ uiMenuAppendSeparator(_menu); }

private:
	uiMenu* _menu;
};

#endif

