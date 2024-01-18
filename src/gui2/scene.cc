#include "scene.h"
#include <QGraphicsSimpleTextItem>

QGraphicsSimpleTextItem* Scene::addAlignedText(qreal x,
    qreal y,
    const QString& text,
    const QFont& font,
    Qt::Alignment alignment)
{
    QGraphicsSimpleTextItem* item = addSimpleText(text, font);
    QRectF bounds = item->boundingRect();

    if (alignment & Qt::AlignHCenter)
        x -= bounds.width() / 2;
    if (alignment & Qt::AlignRight)
        x -= bounds.width();

    if (alignment & Qt::AlignVCenter)
        y -= bounds.height() / 2;
    if (alignment & Qt::AlignBottom)
        y -= bounds.height();

    item->setPos(x, y);
    return item;
}