#pragma once

#include "estoqueitem.h"

#include <QGraphicsItem>

class PalletItem final : public QGraphicsObject {
  Q_OBJECT

public:
  explicit PalletItem(const QRectF size, QGraphicsItem *parent = nullptr);
  auto boundingRect() const -> QRectF override;
  auto getFlagHighlight() const -> bool;
  auto getLabel() const -> QString;
  auto getText() const -> QString;
  auto paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) -> void override;
  auto reorderChildren() -> void;
  auto setFlagHighlight(bool value) -> void;
  auto setLabel(const QString &value) -> void;
  auto setText(const QString &value) -> void;
  auto unselect() -> void;

signals:
  void save();
  void unselectOthers();

private:
  // attributes
  QRectF size;
  QString label;
  QString text;
  bool flagTooltip = false;
  bool flagHighlight = false;
  bool selected = false;
  // methods
  auto select() -> void;

protected:
  auto dragEnterEvent(QGraphicsSceneDragDropEvent *event) -> void override;
  auto dragLeaveEvent(QGraphicsSceneDragDropEvent *event) -> void override;
  auto dragMoveEvent(QGraphicsSceneDragDropEvent *event) -> void override;
  auto dropEvent(QGraphicsSceneDragDropEvent *event) -> void override;
  auto hoverEnterEvent(QGraphicsSceneHoverEvent *event) -> void override;
  auto hoverLeaveEvent(QGraphicsSceneHoverEvent *event) -> void override;
  auto hoverMoveEvent(QGraphicsSceneHoverEvent *event) -> void override;
  auto mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) -> void override;
  auto mouseMoveEvent(QGraphicsSceneMouseEvent *event) -> void override;
  auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void override;
  auto mouseReleaseEvent(QGraphicsSceneMouseEvent *event) -> void override;
};
