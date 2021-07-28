#pragma once

#include <QGraphicsView>

class GraphicsView : public QGraphicsView {
  Q_OBJECT

public:
  explicit GraphicsView(QWidget *parent = nullptr);

protected:
  auto mouseMoveEvent(QMouseEvent *event) -> void override;
  auto mousePressEvent(QMouseEvent *event) -> void override;
  auto mouseReleaseEvent(QMouseEvent *event) -> void override;
  auto resizeEvent(QResizeEvent *event) -> void override;
};
