#pragma once

#include "estoqueitem.h"

#include <QGraphicsItem>

class PalletItem : public QObject, public QGraphicsItem {
  Q_OBJECT

public:
  explicit PalletItem(const QRectF size, QGraphicsItem *parent = nullptr);
  auto getFlagHighlight() const -> bool;
  auto getLabel() const -> QString;
  auto getText() const -> QString;
  auto reorderChildren() -> void;
  auto setFlagHighlight(bool value) -> void;
  auto setLabel(const QString &value) -> void;
  auto setText(const QString &value) -> void;
  auto unselect() -> void;
  virtual auto boundingRect() const -> QRectF override;
  virtual auto paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) -> void override;

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
  virtual auto hoverEnterEvent(QGraphicsSceneHoverEvent *event) -> void override;
  virtual auto hoverMoveEvent(QGraphicsSceneHoverEvent *event) -> void override;
  virtual auto hoverLeaveEvent(QGraphicsSceneHoverEvent *event) -> void override;
  virtual auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void override;
  virtual auto mouseMoveEvent(QGraphicsSceneMouseEvent *event) -> void override;
  virtual auto mouseReleaseEvent(QGraphicsSceneMouseEvent *event) -> void override;
  virtual auto mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) -> void override;
  virtual auto dragEnterEvent(QGraphicsSceneDragDropEvent *event) -> void override;
  virtual auto dragLeaveEvent(QGraphicsSceneDragDropEvent *event) -> void override;
  virtual auto dragMoveEvent(QGraphicsSceneDragDropEvent *event) -> void override;
  virtual auto dropEvent(QGraphicsSceneDragDropEvent *event) -> void override;
};
