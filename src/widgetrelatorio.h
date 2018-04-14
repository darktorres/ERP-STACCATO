#ifndef WIDGETRELATORIO_H
#define WIDGETRELATORIO_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetRelatorio;
}

class WidgetRelatorio final : public Widget {
  Q_OBJECT

public:
  explicit WidgetRelatorio(QWidget *parent = nullptr);
  ~WidgetRelatorio();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelOrcamento;
  SqlRelationalTableModel modelViewRelatorio;
  SqlRelationalTableModel modelViewRelatorioLoja;
  SqlRelationalTableModel modelViewRelatorioVendedor;
  Ui::WidgetRelatorio *ui;
  // methods
  auto calcularTotalGeral() -> void;
  auto dateEditMes_dateChanged(const QDate &) -> void;
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName) -> bool;
  auto on_pushButtonExcel_clicked() -> void;
  auto on_tableRelatorio_entered(const QModelIndex &) -> void;
  auto on_tableTotalLoja_entered(const QModelIndex &) -> void;
  auto on_tableTotalVendedor_entered(const QModelIndex &) -> void;
  auto setFilterRelatorio() -> void;
  auto setFilterTotaisLoja() -> void;
  auto setFilterTotaisVendedor() -> void;
  auto setupTables() -> bool;
};

#endif // WIDGETRELATORIO_H
