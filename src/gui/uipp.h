#ifndef UIPP_H
#define UIPP_H

struct UIGridStyle
{
	int xspan = 1;
	int yspan = 1;
	bool hexpand = false;
	uiAlign halign = uiAlignFill;
	bool vexpand = false;
	uiAlign valign = uiAlignFill;
};

class Closeable
{
public:
	~Closeable() {}
};

class UIAllocator
{
public:
	template <class T, typename... Args>
	T* make(Args&&... args)
	{
		auto uptr = std::make_unique<T>(args...);
		T* ptr = uptr.get();
		_pointers.push_back(std::move(uptr));
		return ptr;
	}

private:
	std::vector<std::unique_ptr<Closeable>> _pointers;
};

class UIControl : public Closeable
{
public:
	UIControl(uiControl* control):
		_control(control)
	{}

	virtual ~UIControl()
	{
		if (_owned && _control)
			uiControlDestroy(_control);
	}

	uiControl* control() const
	{
		return _control;
	}

	uiControl* claim()
	{
		_owned = false;
		return _control;
	}

	bool built() const
	{
		return !!_control;
	}

public:
	UIControl* show()    { uiControlShow(claim()); return this; }
	UIControl* hide()    { uiControlHide(control()); return this; }
	UIControl* enable()  { uiControlEnable(control()); return this; }
	UIControl* disable() { uiControlDisable(control()); return this; }

private:
	uiControl* _control;
	bool _owned = true;
	bool _enabled = true;
};

template <class T, class B>
class UITypedControl : public UIControl
{
public:
	UITypedControl(T* control):
		UIControl(uiControl(control))
	{}

	T* typedControl() const
	{
		return (T*) control();
	}
};

template <class B>
class UIBox : public UITypedControl<uiBox, B>
{
public:
	UIBox(uiBox* control):
		UITypedControl<uiBox, B>(control)
	{}

	B* add(bool expand, UIControl* child)
	{
		uiBoxAppend(this->typedControl(), child->claim(), expand);
		return (B*) this;
	}

	B* add(UIControl* child)
	{
		return add(false, child);
	}
};

class UIHBox : public UIBox<UIHBox>
{
public:
	UIHBox():
		UIBox(uiNewHorizontalBox())
	{}
};

class UIVBox : public UIBox<UIVBox>
{
public:
	UIVBox():
		UIBox(uiNewVerticalBox())
	{}
};

class UIGrid : public UITypedControl<uiGrid, UIGrid>
{
public:
	UIGrid():
		UITypedControl<uiGrid, UIGrid>(uiNewGrid())
	{}

	UIGrid* add(int x, int y, const UIGridStyle& style, UIControl* child)
	{
		uiGridAppend(this->typedControl(), child->claim(),
			x, y,
			style.xspan, style.yspan,
			style.hexpand, style.halign,
			style.vexpand, style.valign);
		return this;
	}

	UIGrid* setPadding(int padding)
	{
		uiGridSetPadded(this->typedControl(), padding);
		return this;
	}
};

class UILabel : public UITypedControl<uiLabel, UILabel>
{
public:
	UILabel(const std::string& text):
		UITypedControl(uiNewLabel(text.c_str()))
	{}
};

class UITextEntry : public UITypedControl<uiEntry, UITextEntry>
{
public:
	UITextEntry():
		UITypedControl(uiNewEntry())
	{}
};

class UICheckBox : public UITypedControl<uiCheckbox, UICheckBox>
{
public:
	UICheckBox(const std::string& text):
		UITypedControl(uiNewCheckbox(text.c_str()))
	{}
};

class UIArea : public UITypedControl<uiArea, UIArea>
{
public:
	UIArea():
		UITypedControl(uiNewArea(&_handler)),
		_selfptr(this)
	{}

	UIArea* setSize(int width, int height)
	{
		uiAreaSetSize(this->typedControl(), width, height);
		return this;
	}

public:
	UIArea* setOnDraw(std::function<void(uiAreaDrawParams*)> cb)
	{ _onRedraw = cb; return this; }

	UIArea* setOnMouseEvent(std::function<void(uiAreaMouseEvent*)> cb)
	{ _onMouseEvent = cb; return this; }

	UIArea* setOnMouseEntryExit(std::function<void(bool)> cb)
	{ _onMouseEntryExit = cb; return this; }

	UIArea* setOnDragBroken(std::function<void()> cb)
	{ _onDragBroken = cb; return this; }

	UIArea* setOnKeyEvent(std::function<int(uiAreaKeyEvent*)> cb)
	{ _onKeyEvent = cb; return this; }

private:
	std::function<void(uiAreaDrawParams*)> _onRedraw;
	std::function<void(uiAreaMouseEvent*)> _onMouseEvent;
	std::function<void(bool)> _onMouseEntryExit;
	std::function<void()> _onDragBroken;
	std::function<int(uiAreaKeyEvent*)> _onKeyEvent;

protected:
	virtual void onRedraw(uiAreaDrawParams* params)
	{ if (_onRedraw) _onRedraw(params); }

	virtual void onMouseEvent(uiAreaMouseEvent* event)
	{ if (_onMouseEvent) _onMouseEvent(event); }

	virtual void onMouseEntryExit(bool exit)
	{ if (_onMouseEntryExit) _onMouseEntryExit(exit); }

	virtual void onDragBroken()
	{ if (_onDragBroken) _onDragBroken(); }

	virtual int onKeyEvent(uiAreaKeyEvent* event)
	{ if (_onKeyEvent) return _onKeyEvent(event); else return 0; }

private:
	static UIArea* _getarea(uiAreaHandler* a)
	{
		return *(UIArea**)(a+1);
	}

	static void _draw_cb(uiAreaHandler* a, uiArea* area, uiAreaDrawParams* p)
	{
		_getarea(a)->onRedraw(p);
	}

	static void _mouse_event_cb(uiAreaHandler* a, uiArea* area, uiAreaMouseEvent* event)
	{
		_getarea(a)->onMouseEvent(event);
	}

	static void _mouse_crossed_cb(uiAreaHandler* a, uiArea* area, int left)
	{
		_getarea(a)->onMouseEntryExit(!left);
	}

	static void _drag_broken_cb(uiAreaHandler* a, uiArea* area)
	{
		_getarea(a)->onDragBroken();
	}

	static int _key_event_cb(uiAreaHandler* a, uiArea* area, uiAreaKeyEvent* event)
	{
		return _getarea(a)->onKeyEvent(event);
	}

	/* These two fields must be next to each other to allow _getarea to find the area
	 * instance pointer. */
	uiAreaHandler _handler = {
		_draw_cb,
		_mouse_event_cb,
		_mouse_crossed_cb,
		_drag_broken_cb,
		_key_event_cb
	};
	UIArea* _selfptr;
};

class UIWindow : public UITypedControl<uiWindow, UIWindow>
{
public:
	UIWindow(const std::string& title, double width, double height):
		UITypedControl<uiWindow, UIWindow>(uiNewWindow(title.c_str(), width, height, 1))
	{
		uiWindowOnClosing(this->typedControl(), _close_cb, this);
	}

	UIWindow* setChild(UIControl* child)
	{
		uiWindowSetChild(this->typedControl(), child->claim());
		return this;
	}

	UIWindow* setOnClose(std::function<int()> cb)
	{
		_onClose = cb;
		return this;
	}

private:
	static int _close_cb(uiWindow*, void* ptr)
	{
		auto cb = ((UIWindow*)ptr)->_onClose;
		if (cb)
			return cb();
		return 0;
	}

private:
	std::string _title;
	std::function<int()> _onClose;
};

class UIButton : public UITypedControl<uiButton, UIButton>
{
public:
	UIButton(const std::string& text):
		UITypedControl(uiNewButton(text.c_str()))
	{}

	UIButton* setText(const std::string& text)
	{
		uiButtonSetText(this->typedControl(), text.c_str());
		return this;
	}

	UIButton* setOnClick(const std::function<void(void)>& onClick)
	{
		_onclick = onClick;
		return this;
	}

private:
	static void _clicked_cb(uiButton*, void* ptr)
	{
		const std::function<void(void)> cb = ((UIButton*)ptr)->_onclick;
		if (cb)
			cb();
	}

private:
	std::string _text;
	std::function<void(void)> _onclick;
};

template <typename T>
class UISelect : public UITypedControl<uiCombobox, UISelect<T>>
{
public:
	UISelect():
		UITypedControl<uiCombobox, UISelect<T>>(uiNewCombobox())
	{
		uiComboboxOnSelected(this->typedControl(), _selected_cb, this);
	}

	UISelect<T>* setOptions(const std::vector<std::pair<std::string, T>>& options)
	{
		assert(_forwards.empty());
		int count = 0;
		for (auto& p : options)
		{
			uiComboboxAppend(this->typedControl(), p.first.c_str());
			_forwards[count] = p.second;
			_backwards[p.second] = count;
			count++;
		}
		return this;
	}

	UISelect<T>* select(T item)
	{
		auto it = _backwards.find(item);
		if (it != _backwards.end())
			uiComboboxSelect(this->typedControl(), it->second);
		return this;
	}

	std::optional<T> getSelected()
	{
		int i = uiComboboxSelected(this->typedControl());
		auto it = _forwards.find(i);
		if (it != _forwards.end())
			return std::make_optional<T>(it->second);
		return std::make_optional<T>();
	}

private:
	static void _selected_cb(uiCombobox*, void* p)
	{
		auto c = ((UISelect<T>*)p);
		if (c->_onselected)
		{
			auto value = c->getSelected();
			if (value)
				c->_onselected(*value);
		}
	}

private:
	std::map<int, T> _forwards;
	std::map<T, int> _backwards;
	std::function<void(T)> _onselected;
};

class UICombo : public UITypedControl<uiEditableCombobox, UICombo>
{
public:
	UICombo():
		UITypedControl<uiEditableCombobox, UICombo>(uiNewEditableCombobox())
	{
		uiEditableComboboxOnChanged(this->typedControl(), _changed_cb, this);
	}

	UICombo* setOptions(const std::vector<std::string>& options)
	{
		for (auto& p : options)
			uiEditableComboboxAppend(this->typedControl(), p.c_str());
		return this;
	}

	UICombo* setText(const std::string& text)
	{
		uiEditableComboboxSetText(this->typedControl(), text.c_str());
		return this;
	}

	std::string getText()
	{
		return uiEditableComboboxText(this->typedControl());
	}

private:
	static void _changed_cb(uiEditableCombobox*, void* p)
	{
		auto c = ((UICombo*)p);
		if (c->_onchanged)
		{
			auto value = c->getText();
			c->_onchanged(value);
		}
	}

private:
	std::function<void(const std::string&)> _onchanged;
};

class UITab : public UITypedControl<uiTab, UITab>
{
public:
	UITab():
		UITypedControl<uiTab, UITab>(uiNewTab())
	{}

	UITab* add(const std::string& name, UIControl* control)
	{
		uiTabAppend(this->typedControl(), name.c_str(), control->claim());
		return this;
	}
};

class UIForm : public UITypedControl<uiForm, UIForm>
{
public:
	UIForm():
		UITypedControl<uiForm, UIForm>(uiNewForm())
	{}

	UIForm* add(const std::string& name, bool expand, UIControl* control)
	{
		uiFormAppend(this->typedControl(), name.c_str(), control->claim(), expand);
		return this;
	}

	UIForm* setPadding(int padding)
	{
		uiFormSetPadded(this->typedControl(), padding);
		return this;
	}
};

#endif

