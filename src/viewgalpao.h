#pragma once

#include "palletitem.h"
#include "widgetgalpao.h"

#include <QGraphicsView>

class ViewGalpao final : public QGraphicsView {
  Q_OBJECT

public:
  explicit ViewGalpao(QWidget *parent = nullptr);

  auto setIsEditable(bool newIsEditable) -> void;
  auto setResizable(bool newResizable) -> void;

signals:
  void selectBloco(PalletItem *item);
  void unselectBloco();

protected:
  auto mouseMoveEvent(QMouseEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;
  auto mouseReleaseEvent(QMouseEvent *event) -> void final;
  auto resizeEvent(QResizeEvent *event) -> void final;
  auto viewportEvent(QEvent *event) -> bool final;
  auto wheelEvent(QWheelEvent *event) -> void final;

private:
  // attributes
  bool resizable = false;
  bool isEditable = false;
  int zoom = 0;
  PalletItem *pallet = nullptr;
  // methods
};
