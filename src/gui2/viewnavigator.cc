#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/fluxmap.h"
#include "lib/data/flux.h"
#include "lib/data/layout.h"
#include "lib/data/fluxmapreader.h"
#include "globals.h"
#include "fluxvisualiserwidget.h"
#include "fluxview.h"
#include "scene.h"
#include "viewnavigator.h"
#include <QWheelEvent>

class ViewNavigatorImpl : public ViewNavigator
{
public:
    ViewNavigatorImpl(QWidget* obj, double xpower): _obj(obj), _xpower(xpower)
    {
        _obj->installEventFilter(this);
    }

    bool eventFilter(QObject* dest, QEvent* event)
    {
        if (dest == _obj)
        {
            switch (event->type())
            {
                case QEvent::MouseButtonPress:
                    mousePressEvent((QMouseEvent*)event);
                    return true;

                case QEvent::MouseMove:
                    mouseMoveEvent((QMouseEvent*)event);
                    return true;

                case QEvent::MouseButtonRelease:
                    mouseReleaseEvent((QMouseEvent*)event);
                    return true;

                case QEvent::Wheel:
                    mouseWheelEvent((QWheelEvent*)event);
                    return true;
            }
        }
        return QObject::eventFilter(dest, event);
    }

private:
    void mousePressEvent(QMouseEvent* event)
    {
        event->accept();
        _reference = event->pos();
        app->setOverrideCursor(Qt::ClosedHandCursor);
        _obj->setMouseTracking(true);
    }

    void mouseMoveEvent(QMouseEvent* event)
    {
        event->accept();
        QPointF d = event->pos() - _reference;
        _delta += {d.x() * 1.0 / xscale(), d.y() * 1.0 / yscale()};
        _reference = event->pos();
        _obj->update();
    }

    void mouseReleaseEvent(QMouseEvent* event)
    {
        event->accept();
        app->restoreOverrideCursor();
        _obj->setMouseTracking(false);
    }

    void mouseWheelEvent(QWheelEvent* event)
    {
        event->accept();

        QPointF pos = event->position() - _obj->rect().center();
        _delta -= {pos.x() / xscale(), pos.y() / yscale()};
        double amount = event->angleDelta().y() / 100.0;
        if (amount > 0)
            _scale *= amount;
        else
            _scale /= -amount;
        _delta += {pos.x() / xscale(), pos.y() / yscale()};
        _obj->update();
    }

public:
    void setScale(double scale) override
    {
        _scale = scale;
        _obj->update();
    }

    void setOrigin(double x, double y)
    {
        _delta = {-x, -y};
        _obj->update();
    }

public:
    void transform(QPainter& painter) override
    {
        painter.translate(_obj->rect().center());
        painter.scale(xscale(), yscale());
        painter.translate(_delta);
    }

private:
    double xscale() const
    {
        return pow(_scale, std::clamp(_xpower * _scale * 0.5, 1.0, _xpower));
    }

    double yscale() const
    {
        return _scale;
    }

private:
    QWidget* _obj;
    double _xpower;
    QRectF _rect;
    QPointF _reference;
    QPointF _delta;
    double _scale = 1.0;
};

std::unique_ptr<ViewNavigator> ViewNavigator::create(
    QWidget* obj, double xpower)
{
    return std::make_unique<ViewNavigatorImpl>(obj, xpower);
}