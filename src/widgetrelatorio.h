#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetRelatorio;
}

class WidgetRelatorio final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetRelatorio(QWidget *parent);
  ~WidgetRelatorio() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelViewRelatorio;
  SqlQueryModel modelViewRelatorioLoja;
  SqlQueryModel modelViewRelatorioVendedor;
  Ui::WidgetRelatorio *ui;
  // methods
  auto calcularTotalGeral() -> void;
  auto dateEditMes_dateChanged() -> void;
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName) -> void;
  auto on_pushButtonExcel_clicked() -> void;
  auto setConnections() -> void;
  auto setFilterRelatorio() -> void;
  auto setFilterTotaisLoja() -> void;
  auto setFilterTotaisVendedor() -> void;
  auto setupTables() -> void;
};
