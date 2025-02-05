#pragma once

class ViewNavigator : public QObject
{
public:
    static std::unique_ptr<ViewNavigator> create(QWidget* obj, double xpower);

public:
    virtual void setScale(double scale) = 0;
    virtual void setOrigin(double x, double y) = 0;

public:
    virtual void transform(QPainter& painter) = 0;
};
