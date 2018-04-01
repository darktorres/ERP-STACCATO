#ifndef WIDGETCOMPRAFATURAR_H
#define WIDGETCOMPRAFATURAR_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetCompraFaturar;
}

class WidgetCompraFaturar final : public Widget {
  Q_OBJECT

public:
  explicit WidgetCompraFaturar(QWidget *parent = nullptr);
  ~WidgetCompraFaturar();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelViewFaturamento;
  SqlRelationalTableModel modelResumo;
  Ui::WidgetCompraFaturar *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> bool;
  auto faturarRepresentacao(const QDateTime &dataReal, const QStringList &idsCompra) -> bool;
  auto montaFiltro() -> void;
  auto on_pushButtonCancelarCompra_clicked() -> void;
  auto on_pushButtonMarcarFaturado_clicked() -> void;
  auto on_pushButtonReagendar_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setupTables() -> void;
};

#endif // WIDGETCOMPRAFATURAR_H
