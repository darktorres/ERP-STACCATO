#ifndef WIDGETFINANCEIROCOMPRA_H
#define WIDGETFINANCEIROCOMPRA_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetFinanceiroCompra;
}

class WidgetFinanceiroCompra final : public Widget {
  Q_OBJECT

public:
  explicit WidgetFinanceiroCompra(QWidget *parent = nullptr);
  ~WidgetFinanceiroCompra();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::WidgetFinanceiroCompra *ui;
  // methods
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setupTables() -> void;
};

#endif // WIDGETFINANCEIROCOMPRA_H
