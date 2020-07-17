#pragma once

#include "sqlquerymodel.h"

#include <QWidget>

namespace Ui {
class WidgetFinanceiroContas;
}

class WidgetFinanceiroContas final : public QWidget {
  Q_OBJECT

public:
  enum class Tipo { Nulo, Receber, Pagar };
  explicit WidgetFinanceiroContas(QWidget *parent);
  ~WidgetFinanceiroContas();
  auto resetTables() -> void;
  auto setTipo(const Tipo &novoTipo) -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlQueryModel model;
  SqlQueryModel modelVencidos;
  SqlQueryModel modelVencer;
  Tipo tipo = Tipo::Nulo;
  Ui::WidgetFinanceiroContas *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_dateEditDe_dateChanged(const QDate &date) -> void;
  auto on_doubleSpinBoxDe_valueChanged(const double value) -> void;
  auto on_groupBoxData_toggled(const bool enabled) -> void;
  auto on_pushButtonAdiantarRecebimento_clicked() -> void;
  auto on_pushButtonExcluirLancamento_clicked() -> void;
  auto on_pushButtonInserirLancamento_clicked() -> void;
  auto on_pushButtonInserirTransferencia_clicked() -> void;
  auto on_pushButtonReverterPagamento_clicked() -> void;
  auto on_tableVencer_doubleClicked(const QModelIndex &index) -> void;
  auto on_tableVencidos_doubleClicked(const QModelIndex &index) -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
