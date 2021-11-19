#include "viewgalpao.h"
#include "palletitem.h"

#include <QDebug>
#include <QMouseEvent>

ViewGalpao::ViewGalpao(QWidget *parent) : QGraphicsView(parent) {}

void ViewGalpao::mousePressEvent(QMouseEvent *event) {
  //  qDebug() << "GraphicsView::mousePressEvent";

  if (isEditable) {
    pallet = new PalletItem("", "", mapToScene(event->pos()), QRectF(0, 0, 40, 40));

    connect(pallet, &PalletItem::selectBloco, this, &ViewGalpao::selectBloco);
    connect(pallet, &PalletItem::unselectBloco, this, &ViewGalpao::unselectBloco);

    scene()->addItem(pallet);
  }

  QGraphicsView::mousePressEvent(event);
}

void ViewGalpao::mouseMoveEvent(QMouseEvent *event) {
  //  qDebug() << "GraphicsView::mouseMoveEvent";

  // FIXME: quando o mouse é movido acima e/ou a esquerda do ponto inicial, o pallet fica invisivel (o tamanho fica negativo)

  if (pallet) { pallet->setSize(QRectF(QPointF(0, 0), mapToScene(event->pos()) - pallet->pos())); }

  QGraphicsView::mouseMoveEvent(event);
}

void ViewGalpao::mouseReleaseEvent(QMouseEvent *event) {
  //  qDebug() << "GraphicsView::mouseReleaseEvent";

  if (pallet) { pallet = nullptr; }

  QGraphicsView::mouseReleaseEvent(event);
}

void ViewGalpao::resizeEvent(QResizeEvent *event) {
  //  qDebug() << "GraphicsView::resizeEvent";

  //  if (resizable and scene()) { fitInView(sceneRect(), Qt::KeepAspectRatio); }

  QGraphicsView::resizeEvent(event);
}

void ViewGalpao::wheelEvent(QWheelEvent *event) {
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

void ViewGalpao::setResizable(bool newResizable) { resizable = newResizable; }

void ViewGalpao::setIsEditable(bool newIsEditable) { isEditable = newIsEditable; }

bool ViewGalpao::viewportEvent(QEvent *event) {
  // TODO: usar essa funcao para receber TouchEvent

  qDebug() << "event: " << event;

  return QGraphicsView::viewportEvent(event);
}
