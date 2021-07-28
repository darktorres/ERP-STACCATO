#include "palletitem.h"

#include "application.h"
#include "sqlquery.h"

#include <QDebug>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>
#include <QSqlError>
#include <QToolTip>

PalletItem::PalletItem(const QRectF &size, QGraphicsItem *parent) : QGraphicsObject(parent), size(size) {
  setAcceptHoverEvents(true);
  setAcceptDrops(true);
}

QRectF PalletItem::boundingRect() const { return size; }

void PalletItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  Q_UNUSED(option)
  Q_UNUSED(widget)

  if (flagHighlight) {
    painter->setPen(QColor(Qt::blue));
    painter->setBrush(QBrush(QColor(Qt::black)));
    painter->drawRect(size);
  } else {
    painter->setPen(QColor(Qt::black));
    painter->drawRect(size);
  }

  if (selected) {
    painter->setBrush(QBrush(QColor(Qt::black), Qt::SolidPattern));
    painter->drawRect(size);
  }

  QFont font = painter->font();
  font.setPixelSize(8);
  painter->setFont(font);
  painter->setPen(QColor(Qt::red));
  painter->drawText(0, size.center().y(), label);
}

void PalletItem::setText(const QString &value) {
  const auto lines = value.split("\n", Qt::SkipEmptyParts);

  int pos = 15;

  for (const auto &line : lines) {
    auto estoque = new EstoqueItem(line, line.split(" - ").last().toInt(), this);
    estoque->setVisible(false);
    estoque->setPos(mapFromScene(680, pos));
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

  const auto children = childItems();

  for (const auto estoque : children) {
    estoque->setPos(mapFromScene(680, pos));
    pos += 15;
  }
}

void PalletItem::unselect() {
  if (not selected) { return; }

  selected = false;

  prepareGeometryChange();

  const auto children = childItems();

  for (const auto estoque : children) { estoque->setVisible(false); }
}

void PalletItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) { QGraphicsItem::hoverEnterEvent(event); }

void PalletItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) { QGraphicsItem::hoverMoveEvent(event); }

void PalletItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) { QGraphicsItem::hoverLeaveEvent(event); }

void PalletItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if (flags().testFlag(QGraphicsItem::ItemIsMovable)) { return; }

  if (not selected) { emit unselectOthers(); }

  selected = not selected;

  prepareGeometryChange();

  const auto children = childItems();

  for (const auto estoque : children) { estoque->setVisible(not estoque->isVisible()); }

  QGraphicsItem::mousePressEvent(event);
}

void PalletItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) { QGraphicsItem::mouseMoveEvent(event); }

void PalletItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (flags().testFlag(QGraphicsItem::ItemIsMovable)) { emit save(); }

  reorderChildren();

  QGraphicsItem::mouseReleaseEvent(event);
}

void PalletItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) { QGraphicsItem::mouseDoubleClickEvent(event); }

void PalletItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
  if (childItems().contains(qobject_cast<EstoqueItem *>(event->mimeData()->parent()))) {
    event->ignore();
    return;
  }

  if (event->mimeData()->hasFormat("text/plain")) { event->accept(); }
}

void PalletItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *event) { Q_UNUSED(event); }

void PalletItem::dragMoveEvent(QGraphicsSceneDragDropEvent *event) { Q_UNUSED(event); }

void PalletItem::dropEvent(QGraphicsSceneDragDropEvent *event) {
  Q_UNUSED(event);

  const QStringList text = event->mimeData()->text().split(" - ", Qt::SkipEmptyParts);

  if (text.isEmpty()) { return; }

  const QString id = text.at(0);
  const QString table = (text.at(1) == "ESTOQUE") ? "estoque" : "estoque_has_consumo";
  const QString idName = (table == "estoque") ? "idEstoque" : "idConsumo";

  SqlQuery query;

  if (not query.exec("UPDATE " + table + " SET bloco = '" + label + "' WHERE " + idName + " = " + id)) {
    event->ignore();
    throw RuntimeError("Erro movendo estoque: " + query.lastError().text());
  }

  EstoqueItem *item = qobject_cast<EstoqueItem *>(event->mimeData()->parent());
  item->setParentItem(this);
  item->setVisible(false);
  reorderChildren();

  event->acceptProposedAction();
}

void PalletItem::select() {
  selected = not selected;

  prepareGeometryChange();

  const auto children = childItems();

  for (const auto estoque : children) { estoque->setVisible(not estoque->isVisible()); }
}
