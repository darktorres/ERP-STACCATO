#pragma once

#include <QGraphicsView>

class GraphicsView final : public QGraphicsView {
  Q_OBJECT

public:
  explicit GraphicsView(QWidget *parent = nullptr);

protected:
  auto mouseMoveEvent(QMouseEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;
  auto mouseReleaseEvent(QMouseEvent *event) -> void final;
  auto resizeEvent(QResizeEvent *event) -> void final;
};
