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
  //  qDebug() << "GraphicsView::mousePressEvent";

  if (isEditable) {
    pallet = new PalletItem("", "", mapToScene(event->pos()), QRectF(0, 0, 40, 40), sceneRect().width());

    connect(pallet, &PalletItem::save, widgetGalpao, &WidgetGalpao::salvarPallets);
    connect(pallet, &PalletItem::unselectOthers, widgetGalpao, &WidgetGalpao::unselectOthers);
    connect(pallet, &PalletItem::selectBloco, widgetGalpao, &WidgetGalpao::carregarBloco);
    connect(pallet, &PalletItem::unselectBloco, widgetGalpao, &WidgetGalpao::unselectBloco);

    scene()->addItem(pallet);
  }

  QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event) {
  //  qDebug() << "GraphicsView::mouseMoveEvent";

  if (pallet) { pallet->setSize(QRectF(QPointF(0, 0), mapToScene(event->pos()) - pallet->pos())); }

  QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event) {
  //  qDebug() << "GraphicsView::mouseReleaseEvent";

  if (pallet) { pallet = nullptr; }

  QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::resizeEvent(QResizeEvent *event) {
  //  qDebug() << "GraphicsView::resizeEvent";

  //  if (resizable and scene()) { fitInView(sceneRect(), Qt::KeepAspectRatio); }

  QGraphicsView::resizeEvent(event);
}

void GraphicsView::wheelEvent(QWheelEvent *event) {
  //  qDebug() << "GraphicsView::wheelEvent";

  // TODO: implementar zoom por touch

  double factor = 0;

  if (event->angleDelta().y() > 0) {
    if (zoom == 6) { return; }

    factor = 1.25;
    zoom += 1;
  } else {
    if (zoom == -6) { return; }

    factor = 0.8;
    zoom -= 1;
  }

  scale(factor, factor);

  // TODO: accept event?

  QGraphicsView::wheelEvent(event);
}

void GraphicsView::setResizable(bool newResizable) { resizable = newResizable; }

void GraphicsView::setIsEditable(bool newIsEditable) { isEditable = newIsEditable; }
