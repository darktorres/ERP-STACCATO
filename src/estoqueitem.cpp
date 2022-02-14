#include "estoqueitem.h"

#include <QDebug>
#include <QDrag>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>

EstoqueItem::EstoqueItem(const QString &text, QGraphicsItem *parent) : QGraphicsSimpleTextItem(text, parent), idVendaProduto2(text.split(" - ").last().toInt()) { setBrush(QBrush(QColor(Qt::red))); }

void EstoqueItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  startDrag(event->pos());

  QGraphicsSimpleTextItem::mousePressEvent(event);
}

// TODO: transferir esse código para WidgetGalpao e apagar essa classe
void EstoqueItem::startDrag(QPointF pos) {
  Q_UNUSED(pos)

  emit startDragSignal();

  QPixmap pixmap = QPixmap("://box_medium.png");

  auto *mimeData = new QMimeData;
  mimeData->setText(text());
  mimeData->setParent(this);

  auto *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setPixmap(pixmap);

  drag->exec();
}

int EstoqueItem::getIdVendaProduto2() const { return idVendaProduto2; }
