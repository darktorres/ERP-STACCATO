#ifndef WIDGETCOMPRA_H
#define WIDGETCOMPRA_H

#include <QWidget>

namespace Ui {
class WidgetCompra;
}

class WidgetCompra final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompra(QWidget *parent = nullptr);
  ~WidgetCompra();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  Ui::WidgetCompra *ui;
  // methods
  auto on_tabWidget_currentChanged(const int &) -> void;
  auto setConnections() -> void;
};

#endif // WIDGETCOMPRA_H
