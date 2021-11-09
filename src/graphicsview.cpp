#include "graphicsview.h"
#include "palletitem.h"

#include <QDebug>
#include <QMouseEvent>

GraphicsView::GraphicsView(QWidget *parent) : QGraphicsView(parent) {
  setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
  setDragMode(QGraphicsView::ScrollHandDrag);

  setTransformationAnchor(AnchorUnderMouse);
  setResizeAnchor(AnchorUnderMouse);

  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  setFrameShape(NoFrame);
}

void GraphicsView::mousePressEvent(QMouseEvent *event) {
  //  qDebug() << "press";

  if (isEditable) {
    // TODO: verificar quantos PalletItem tem na scene e usar a quantidade como label

    pallet = new PalletItem("", "", mapToScene(event->pos()), QRectF(0, 0, 40, 40), sceneRect().width());

    scene()->addItem(pallet);
  }

  QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event) {
  //  qDebug() << "move";

  if (pallet) { pallet->setSize(QRectF(QPointF(0, 0), mapToScene(event->pos()) - pallet->pos())); }

  QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event) {
  //  qDebug() << "release";

  if (pallet) { pallet = nullptr; }

  QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::resizeEvent(QResizeEvent *event) {
  //  qDebug() << "view resize: " << event;

  //  if (resizable and scene()) { fitInView(sceneRect(), Qt::KeepAspectRatio); }

  QGraphicsView::resizeEvent(event);
}

void GraphicsView::setResizable(bool newResizable) { resizable = newResizable; }

void GraphicsView::setIsEditable(bool newIsEditable) { isEditable = newIsEditable; }

void GraphicsView::wheelEvent(QWheelEvent *event) {
  double factor = 0;

  if (event->angleDelta().y() > 0) {
    factor = 1.25;
    zoom += 1;
  } else {
    factor = 0.8;
    zoom -= 1;
  }

  scale(factor, factor);
}
