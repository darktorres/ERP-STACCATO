#pragma once

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraPendentes;
}

class WidgetCompraPendentes final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraPendentes(QWidget *parent = nullptr);
  ~WidgetCompraPendentes();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewVendaProduto;
  Ui::WidgetCompraPendentes *ui;
  // methods
  auto insere(const QDate &dataPrevista) -> bool;
  auto setConnections() -> void;
  auto montaFiltro() -> void;
  auto on_doubleSpinBoxQuantAvulsoCaixas_valueChanged(const double value) -> void;
  auto on_doubleSpinBoxQuantAvulso_valueChanged(const double value) -> void;
  auto on_groupBoxStatus_toggled(bool enabled) -> void;
  auto on_pushButtonComprarAvulso_clicked() -> void;
  auto on_pushButtonExcel_clicked() -> void;
  auto on_pushButtonPDF_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setarDadosAvulso() -> void;
  auto setupTables() -> void;
};
