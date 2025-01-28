#pragma once

class ViewNavigator : public QObject
{
public:
    static std::unique_ptr<ViewNavigator> create(QWidget* obj);

public:
    virtual void transform(QPainter& painter)  = 0;
};
