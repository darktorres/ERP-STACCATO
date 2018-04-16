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
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewComprasFinanceiro;
  Ui::WidgetFinanceiroCompra *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETFINANCEIROCOMPRA_H
