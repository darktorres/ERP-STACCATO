#pragma once

#include <QGraphicsTextItem>

class EstoqueItem : public QObject, public QGraphicsSimpleTextItem {
  Q_OBJECT

public:
  explicit EstoqueItem(const QString &text, const int idVendaProduto2, QGraphicsItem *parent = nullptr);

  // TODO: make this private?
  int const idVendaProduto2;

signals:
  void startDragSignal();

protected:
  auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto startDrag(QPointF pos) -> void;
};
