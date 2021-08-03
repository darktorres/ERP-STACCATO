#pragma once

#include "estoqueitem.h"

#include <QGraphicsItem>

class PalletItem final : public QGraphicsObject {
  Q_OBJECT

public:
  explicit PalletItem(const QRectF &size, QGraphicsItem *parent = nullptr);

  auto boundingRect() const -> QRectF final;
  auto getFlagHighlight() const -> bool;
  auto getLabel() const -> QString;
  auto getText() const -> QString;
  auto paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) -> void final;
  auto reorderChildren() -> void;
  auto setFlagHighlight(const bool value) -> void;
  auto setLabel(const QString &value) -> void;
  auto setText(const QString &value) -> void;
  auto unselect() -> void;

signals:
  void save();
  void unselectOthers();

protected:
  auto dragEnterEvent(QGraphicsSceneDragDropEvent *event) -> void final;
  auto dragLeaveEvent(QGraphicsSceneDragDropEvent *event) -> void final;
  auto dragMoveEvent(QGraphicsSceneDragDropEvent *event) -> void final;
  auto dropEvent(QGraphicsSceneDragDropEvent *event) -> void final;
  auto hoverEnterEvent(QGraphicsSceneHoverEvent *event) -> void final;
  auto hoverLeaveEvent(QGraphicsSceneHoverEvent *event) -> void final;
  auto hoverMoveEvent(QGraphicsSceneHoverEvent *event) -> void final;
  auto mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto mouseMoveEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto mouseReleaseEvent(QGraphicsSceneMouseEvent *event) -> void final;

private:
  // attributes
  bool flagTooltip = false;
  bool flagHighlight = false;
  bool selected = false;
  QRectF size;
  QString label;
  QString text;
  // methods
  auto select() -> void;
};
