#pragma once

#include <QDialog>

#include "sortfilterproxymodel.h"
#include "sqlrelationaltablemodel.h"
#include "sqltreemodel.h"

namespace Ui {
class InputDialogFinanceiro;
}

class InputDialogFinanceiro final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { ConfirmarCompra, Financeiro };

  explicit InputDialogFinanceiro(const Tipo &tipo, QWidget *parent = nullptr);
  ~InputDialogFinanceiro();
  auto getDate() const -> QDate;
  auto getNextDate() const -> QDate;
  auto setFilter(const QString &idCompra) -> bool;

private:
  // attributes
  bool representacao;
  const Tipo tipo;
  SqlRelationalTableModel modelPedidoFornecedor;
  SqlRelationalTableModel modelPedidoFornecedor2;
  SqlRelationalTableModel modelFluxoCaixa;
  SqlTreeModel modelTree;
  Ui::InputDialogFinanceiro *ui;
  // methods
  auto cadastrar() -> bool;
  auto calcularTotal() -> void;
  auto montarFluxoCaixa(const bool updateDate = true) -> void;
  auto on_checkBoxMarcarTodos_toggled(const bool checked) -> void;
  auto on_comboBoxST_currentTextChanged(const QString &text) -> void;
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_dateEditPgtSt_dateChanged(const QDate &) -> void;
  auto on_doubleSpinBoxAliquota_valueChanged(const double aliquota) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(double) -> void;
  auto on_doubleSpinBoxSt_valueChanged(const double valueSt) -> void;
  auto on_pushButtonCorrigirFluxo_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setTreeView() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
  auto verifyFields() -> bool;
};
