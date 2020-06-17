#pragma once

#include <QGraphicsTextItem>

class EstoqueItem : public QObject, public QGraphicsSimpleTextItem {
  Q_OBJECT

public:
  explicit EstoqueItem(const QString &text, QGraphicsItem *parent = nullptr);

  // QGraphicsItem interface
protected:
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void startDrag(QPointF pos);
};
