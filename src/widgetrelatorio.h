#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetRelatorio;
}

class WidgetRelatorio final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetRelatorio(QWidget *parent);
  ~WidgetRelatorio();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelOrcamento;
  SqlTableModel modelViewRelatorio;
  SqlTableModel modelViewRelatorioLoja;
  SqlTableModel modelViewRelatorioVendedor;
  Ui::WidgetRelatorio *ui;
  // methods
  auto calcularTotalGeral() -> void;
  auto calcularTotalVendedor() -> void;
  auto dateEditMes_dateChanged(const QDate &) -> void;
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName) -> bool;
  auto on_pushButtonExcel_clicked() -> void;
  auto setConnections() -> void;
  auto setFilterRelatorio() -> void;
  auto setFilterTotaisLoja() -> void;
  auto setFilterTotaisVendedor() -> void;
  auto setResumoOrcamento() -> void;
  auto setupTables() -> void;
};
