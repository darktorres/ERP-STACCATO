#pragma once

#include "palletitem.h"

#include <QGraphicsView>

// TODO: renomear para algo especifico do galpao
class GraphicsView final : public QGraphicsView {
  Q_OBJECT

public:
  explicit GraphicsView(QWidget *parent = nullptr);

  auto setIsEditable(bool newIsEditable) -> void;
  auto setResizable(bool newResizable) -> void;

protected:
  auto mouseMoveEvent(QMouseEvent *event) -> void final;
  auto mousePressEvent(QMouseEvent *event) -> void final;
  auto mouseReleaseEvent(QMouseEvent *event) -> void final;
  auto resizeEvent(QResizeEvent *event) -> void final;
  auto wheelEvent(QWheelEvent *event) -> void final;

private:
  // attributes
  bool resizable = false;
  bool isEditable = false;
  int zoom = 0;
  PalletItem *pallet = nullptr;
  // methods
};
