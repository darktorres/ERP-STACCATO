#pragma once

#include <QGraphicsView>

class GraphicsView : public QGraphicsView {

public:
  explicit GraphicsView(QWidget *parent = nullptr);

protected:
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;
  virtual void resizeEvent(QResizeEvent *event) override;
};
