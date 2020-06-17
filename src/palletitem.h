#pragma once

#include "estoqueitem.h"

#include <QGraphicsItem>

class PalletItem : public QObject, public QGraphicsItem {
  Q_OBJECT

public:
  explicit PalletItem(const QRectF size, QGraphicsItem *parent = nullptr);
  virtual QRectF boundingRect() const override;
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
  void setText(const QString &value);
  void setLabel(const QString &value);
  QString getLabel() const;
  QString getText() const;
  bool getFlagHighlight() const;
  void setFlagHighlight(bool value);
  void reorderChildren();

signals:
  void save();

private:
  QRectF size;
  QString label;
  QString text;
  bool flagTooltip = false;
  bool flagHighlight = false;

protected:
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
  virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

  // QGraphicsItem interface
protected:
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

  // QGraphicsItem interface
protected:
  virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event) override;
  virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event) override;
  virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event) override;
  virtual void dropEvent(QGraphicsSceneDragDropEvent *event) override;
};
