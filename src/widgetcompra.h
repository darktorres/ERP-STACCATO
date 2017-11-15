#ifndef WIDGETCOMPRA_H
#define WIDGETCOMPRA_H

#include "widget.h"

namespace Ui {
class WidgetCompra;
}

class WidgetCompra final : public Widget {
  Q_OBJECT

public:
  explicit WidgetCompra(QWidget *parent = 0);
  ~WidgetCompra();
  bool updateTables();
  void setHasError(const bool value);

private slots:
  void on_tabWidget_currentChanged(const int &);

private:
  // attributes
  bool hasError = false;
  Ui::WidgetCompra *ui;
  // methods
  void setConnections();
};

#endif // WIDGETCOMPRA_H
