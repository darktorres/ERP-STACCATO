#include "estoqueitem.h"

#include "palletitem.h"

#include <QDebug>
#include <QDrag>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>

EstoqueItem::EstoqueItem(const QString &text, QGraphicsItem *parent) : QGraphicsSimpleTextItem(text, parent) {}

void EstoqueItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  startDrag(event->pos());

  QGraphicsSimpleTextItem::mousePressEvent(event);
}

void EstoqueItem::startDrag(QPointF pos) {
  Q_UNUSED(pos);

  QPixmap pixmap = QPixmap("://box_medium.png");

  auto pallet = dynamic_cast<PalletItem *>(parentItem());

  QMimeData *mimeData = new QMimeData;
  mimeData->setText(text());
  mimeData->setParent(this);
  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setPixmap(pixmap);
  auto status = drag->exec();

  if (status != Qt::IgnoreAction) { pallet->reorderChildren(); }
}
