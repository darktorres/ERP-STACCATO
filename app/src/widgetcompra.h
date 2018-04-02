#ifndef WIDGETCOMPRA_H
#define WIDGETCOMPRA_H

#include "widget.h"

namespace Ui {
class WidgetCompra;
}

class WidgetCompra final : public Widget {
  Q_OBJECT

public:
  explicit WidgetCompra(QWidget *parent = nullptr);
  ~WidgetCompra();
  auto updateTables() -> bool;

private:
  // attributes
  Ui::WidgetCompra *ui;
  // methods
  auto on_tabWidget_currentChanged(const int &) -> void;
  auto setConnections() -> void;
};

#endif // WIDGETCOMPRA_H
