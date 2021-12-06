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
		_control(control),
		_owned(true)
	{}

	~UIControl()
	{
		if (_owned)
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

private:
	uiControl* _control;
	bool _owned;
};

template <class T>
class UITypedControl : public UIControl
{
public:
	UITypedControl(T* control):
		UIControl(uiControl(control))
	{}

	T* self() const
	{
		return (T*) control();
	}
};

class UIBox : public UITypedControl<uiBox>
{
public:
	UIBox(uiBox* box):
		UITypedControl(box)
	{}

	UIBox& append(UIControl& control, bool stretchy=false)
	{
		uiBoxAppend(self(), control.claim(), stretchy);
		return *this;
	}
};

class UIHBox : public UIBox
{
public:
	UIHBox():
		UIBox(uiNewHorizontalBox())
	{}
};

class UIVBox : public UIBox
{
public:
	UIVBox():
		UIBox(uiNewVerticalBox())
	{}
};

class UIArea : public UITypedControl<uiArea>
{
public:
	UIArea():
		UITypedControl(uiNewArea(&_handler)),
		_selfptr(this)
	{}

	virtual void onRedraw(uiAreaDrawParams* params)
	{
	}

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
	}

	static void _mouse_crossed_cb(uiAreaHandler* a, uiArea* area, int left)
	{
	}

	static void _drag_broken_cb(uiAreaHandler* a, uiArea* area)
	{
	}

	static int _key_event_cb(uiAreaHandler* a, uiArea* area, uiAreaKeyEvent* event)
	{
			return 0;
	}

	uiAreaHandler _handler = {
		_draw_cb,
		_mouse_event_cb,
		_mouse_crossed_cb,
		_drag_broken_cb,
		_key_event_cb
	};
	UIArea* _selfptr;
};

class UIWindow : public UITypedControl<uiWindow>
{
public:
	UIWindow(const std::string& title, double width, double height):
		UITypedControl(uiNewWindow(title.c_str(), width, height, 1))
	{
		uiWindowOnClosing(self(), _close_cb, this);
	}

	UIWindow& setChild(UIControl& control)
	{
		uiWindowSetChild(self(), control.claim());
		return *this;
	}

	UIWindow& show()
	{
		uiControlShow(claim());
		return *this;
	}

	virtual int onClose()
	{
		return 0;
	}

private:
	static int _close_cb(uiWindow*, void* ptr)
	{
		return ((UIWindow*)ptr)->onClose();
	}
};

#endif

