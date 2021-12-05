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

#endif

