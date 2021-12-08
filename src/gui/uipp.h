#ifndef UIPP_H
#define UIPP_H

class UIPath
{
	class Figure
	{
	public:
		Figure(UIPath& path, double x, double y):
			_path(path)
		{
			uiDrawPathNewFigure(path._path, x, y);
		}

		Figure& lineTo(double x, double y)
		{
			uiDrawPathLineTo(_path._path, x, y);
			return *this;
		}

		UIPath& end()
		{
			uiDrawPathCloseFigure(_path._path);
			return _path;
		}

	private:
		UIPath& _path;
	};

public:
	UIPath(uiAreaDrawParams* params):
		_params(params),
		_path(uiDrawNewPath(uiDrawFillModeWinding))
	{}

	UIPath& rectangle(double x, double y, double w, double h)
	{
		uiDrawPathAddRectangle(_path, x, y, w, h);
		return *this;
	}

	Figure begin(double x, double y)
	{
		return Figure(*this, x, y);
	}

	void fill(uiDrawBrush& fillBrush)
	{
		uiDrawPathEnd(_path);
		uiDrawFill(_params->Context, _path, &fillBrush);
	}

	void stroke(uiDrawBrush& strokeBrush, uiDrawStrokeParams& strokeParams)
	{
		uiDrawPathEnd(_path);
		uiDrawStroke(_params->Context, _path, &strokeBrush, &strokeParams);
	}

	void fill(uiDrawBrush& strokeBrush, uiDrawStrokeParams& strokeParams, uiDrawBrush& fillBrush)
	{
		uiDrawPathEnd(_path);
		uiDrawFill(_params->Context, _path, &fillBrush);
		uiDrawStroke(_params->Context, _path, &strokeBrush, &strokeParams);
	}

	~UIPath()
	{
		uiDrawFreePath(_path);
	}
	
private:
	uiAreaDrawParams* _params;
	uiDrawPath* _path;
};

class UIControl
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

	bool stretchy() const
	{
		return _stretchy;
	}

	bool built() const
	{
		return !!_control;
	}

public:
	UIControl* setStretchy(bool stretchy) { _stretchy = stretchy; return this; }

	UIControl* show()    { uiControlShow(claim()); return this; }
	UIControl* hide()    { uiControlHide(control()); return this; }
	UIControl* enable()  { uiControlEnable(control()); return this; }
	UIControl* disable() { uiControlDisable(control()); return this; }

private:
	uiControl* _control;
	bool _owned = true;
	bool _stretchy = false;
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

template <class T, class B>
class UIContainerControl : public UITypedControl<T, B>
{
public:
	UIContainerControl(T* control):
		UITypedControl<T, B>(control)
	{}

	B* add(UIControl* child)
	{
		uiBoxAppend(this->typedControl(), child->claim(), child->stretchy());
		_children.push_back(child);
		return (B*) this;
	}

	const std::vector<UIControl*>& children() const { return _children; }

private:
	std::vector<UIControl*> _children;
};

template <class B>
class UIBox : public UIContainerControl<uiBox, B>
{
public:
	UIBox(uiBox* control):
		UIContainerControl<uiBox, B>(control)
	{}
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

class UIArea : public UITypedControl<uiArea, UIArea>
{
public:
	UIArea():
		UITypedControl(uiNewArea(&_handler)),
		_selfptr(this)
	{}

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
class UICombo : public UITypedControl<uiCombobox, UICombo<T>>
{
public:
	UICombo():
		UITypedControl<uiCombobox, UICombo<T>>(uiNewCombobox())
	{
		uiComboboxOnSelected(this->typedControl(), _selected_cb, this);
	}

	UICombo<T>* setOptions(const std::vector<std::pair<std::string, T>>& options)
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

	UICombo<T>* select(T item)
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
		auto combo = ((UICombo<T>*)p);
		if (combo->_onselected)
		{
			auto value = combo->getSelected();
			if (value)
				combo->_onselected(*value);
		}
	}

private:
	std::map<int, T> _forwards;
	std::map<T, int> _backwards;
	std::function<void(T)> _onselected;
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
	std::vector<std::unique_ptr<UIControl>> _pointers;
};

#endif

