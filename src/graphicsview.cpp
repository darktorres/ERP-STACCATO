#include "graphicsview.h"

#include <QDebug>
#include <QMouseEvent>

GraphicsView::GraphicsView(QWidget *parent) : QGraphicsView(parent) {}

void GraphicsView::mousePressEvent(QMouseEvent *event) {
  qDebug() << "pos: " << event->pos();

  QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event) { QGraphicsView::mouseReleaseEvent(event); }

void GraphicsView::mouseMoveEvent(QMouseEvent *event) { QGraphicsView::mouseMoveEvent(event); }

void GraphicsView::resizeEvent(QResizeEvent *event) {
  //  fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);

  QGraphicsView::resizeEvent(event);
}
