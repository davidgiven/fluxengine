#pragma once

#include <QGraphicsScene>

class Scene : public QGraphicsScene
{
public:
    QGraphicsSimpleTextItem* addAlignedText(qreal x,
        qreal y,
        const QString& text,
        const QFont& font = QFont(),
        Qt::Alignment alignment = Qt::AlignHCenter | Qt::AlignVCenter);
};
