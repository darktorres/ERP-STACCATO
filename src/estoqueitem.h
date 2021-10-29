#pragma once

#include <QGraphicsTextItem>

class EstoqueItem : public QObject, public QGraphicsSimpleTextItem {
  Q_OBJECT

public:
  explicit EstoqueItem(const QString &text, QGraphicsItem *parent = nullptr);

  auto getIdVendaProduto2() const -> int;

signals:
  void startDragSignal();

protected:
  auto mousePressEvent(QGraphicsSceneMouseEvent *event) -> void final;
  auto startDrag(QPointF pos) -> void;

private:
  int const idVendaProduto2;
};
