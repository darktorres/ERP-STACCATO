#pragma once

#include "sqltablemodel.h"

#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetCompraPendentes;
}

class WidgetCompraPendentes final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraPendentes(QWidget *parent);
  ~WidgetCompraPendentes();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QStack<int> blockingSignals;
  QTimer timer;
  SqlTableModel modelViewVendaProduto;
  Ui::WidgetCompraPendentes *ui;
  // methods
  auto ajustarGroupBoxStatus() -> void;
  auto delayFiltro() -> void;
  auto insere(const QDate dataPrevista) -> void;
  auto montaFiltro() -> void;
  auto on_doubleSpinBoxAvulsoCaixas_valueChanged(const double caixas) -> void;
  auto on_doubleSpinBoxAvulsoQuant_valueChanged(const double quant) -> void;
  auto on_groupBoxStatus_toggled(const bool enabled) -> void;
  auto on_pushButtonComprarAvulso_clicked() -> void;
  auto on_pushButtonExcel_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_pushButtonPDF_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setarDadosAvulso() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
