#include "palletitem.h"

#include "application.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QPainter>
#include <QSqlError>
#include <QSqlQuery>
#include <QToolTip>

PalletItem::PalletItem(const QRectF size, QGraphicsItem *parent) : QGraphicsItem(parent), size(size) {
  setAcceptHoverEvents(true);
  setAcceptDrops(true);
}

QRectF PalletItem::boundingRect() const { return flagTooltip ? size.united(childrenBoundingRect()) : size; }

void PalletItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option)
  Q_UNUSED(widget)

  if (flagHighlight) {
    painter->setPen(QColor(Qt::blue));
    painter->setBrush(QBrush(QColor(Qt::black)));
    painter->drawRect(size);
  } else {
    //    painter->setPen(QColor(Qt::red));
    //    painter->drawRect(boundingRect());
  }

  setZValue(0);
  QFont font = painter->font();
  font.setPixelSize(8);
  painter->setFont(font);
  painter->setPen(QColor(Qt::red));
  painter->drawText(size.center().x() / 2, size.center().y(), label);

  if (flagTooltip) {
    setZValue(1);
    painter->setBrush(QBrush(QColor(Qt::black), Qt::SolidPattern));
    painter->drawRect(boundingRect());
  }
}

void PalletItem::setText(const QString &value) {
  auto lines = value.split("\n", QString::SkipEmptyParts);

  int pos = 15;

  for (auto line : lines) {
    auto estoque = new EstoqueItem(line, this);
    estoque->setVisible(false);
    estoque->setPos(30, pos);
    estoque->setBrush(QBrush(QColor(Qt::red)));
    pos += 15;
  }

  text = value;
}

void PalletItem::setLabel(const QString &value) { label = value; }

QString PalletItem::getLabel() const { return label; }

QString PalletItem::getText() const { return text; }

bool PalletItem::getFlagHighlight() const { return flagHighlight; }

void PalletItem::setFlagHighlight(bool value) { flagHighlight = value; }

void PalletItem::reorderChildren() {
  int pos = 15;

  for (auto estoque : childItems()) {
    estoque->setPos(30, pos);
    pos += 15;
  }
}

void PalletItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
  flagTooltip = true;

  prepareGeometryChange();
  for (auto estoque : childItems()) { estoque->setVisible(true); }

  QGraphicsItem::hoverEnterEvent(event);
}

void PalletItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) { QGraphicsItem::hoverMoveEvent(event); }

void PalletItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
  flagTooltip = false;

  prepareGeometryChange();
  for (auto estoque : childItems()) { estoque->setVisible(false); }

  QGraphicsItem::hoverLeaveEvent(event);
}

void PalletItem::mousePressEvent(QGraphicsSceneMouseEvent *event) { QGraphicsItem::mousePressEvent(event); }

void PalletItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) { QGraphicsItem::mouseMoveEvent(event); }

void PalletItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  emit save();

  QGraphicsItem::mouseReleaseEvent(event);
}

void PalletItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) { QGraphicsItem::mouseDoubleClickEvent(event); }

void PalletItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
  if (event->mimeData()->hasFormat("text/plain")) { event->acceptProposedAction(); }
}

void PalletItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *event) { Q_UNUSED(event); }

void PalletItem::dragMoveEvent(QGraphicsSceneDragDropEvent *event) { Q_UNUSED(event); }

void PalletItem::dropEvent(QGraphicsSceneDragDropEvent *event) {
  Q_UNUSED(event);

  QSqlQuery query;

  if (not query.exec("UPDATE estoque SET bloco = '" + label + "' WHERE idEstoque = " + event->mimeData()->text().split(" - ").first())) {
    event->ignore();
    return qApp->enqueueError("Erro movendo estoque: " + query.lastError().text());
  }

  EstoqueItem *item = dynamic_cast<EstoqueItem *>(event->mimeData()->parent());
  item->setParentItem(this);
  reorderChildren();

  event->acceptProposedAction();
}
