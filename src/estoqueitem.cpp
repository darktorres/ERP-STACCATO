#include "estoqueitem.h"

#include "palletitem.h"

#include <QDebug>
#include <QDrag>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>

EstoqueItem::EstoqueItem(const QString &text, const int idVendaProduto2, QGraphicsItem *parent) : QGraphicsSimpleTextItem(text, parent), idVendaProduto2(idVendaProduto2) {}

void EstoqueItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  startDrag(event->pos());

  QGraphicsSimpleTextItem::mousePressEvent(event);
}

void EstoqueItem::startDrag(QPointF pos) {
  Q_UNUSED(pos);

  emit startDragSignal();

  QPixmap pixmap = QPixmap("://box_medium.png");

  const auto pallet = dynamic_cast<PalletItem *>(parentItem());

  QMimeData *mimeData = new QMimeData;
  mimeData->setText(text());
  mimeData->setParent(this);

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setPixmap(pixmap);

  const auto status = drag->exec();

  if (status != Qt::IgnoreAction) { pallet->reorderChildren(); }
}
