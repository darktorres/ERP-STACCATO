#ifndef WIDGETLOGISTICAENTREGUES_H
#define WIDGETLOGISTICAENTREGUES_H

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogisticaEntregues;
}

class WidgetLogisticaEntregues final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaEntregues(QWidget *parent = nullptr);
  ~WidgetLogisticaEntregues();
  auto updateTables() -> bool;

private:
  // attributes
  SqlQueryModel modelProdutos;
  SqlRelationalTableModel modelVendas;
  Ui::WidgetLogisticaEntregues *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> bool;
  auto montaFiltro() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_tableVendas_clicked(const QModelIndex &index) -> void;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICAENTREGUES_H
