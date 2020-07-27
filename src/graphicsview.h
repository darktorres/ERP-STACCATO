#pragma once

#include <QGraphicsView>

class GraphicsView : public QGraphicsView {

public:
  explicit GraphicsView(QWidget *parent = nullptr);

  // QWidget interface
protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;

  // QWidget interface
protected:
  virtual void resizeEvent(QResizeEvent *event) override;
};
