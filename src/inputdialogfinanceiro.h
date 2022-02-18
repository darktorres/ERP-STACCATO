#pragma once

#include "sqltablemodel.h"
#include "sqltreemodel.h"
#include "widgetpagamentos.h"

#include <QDialog>
#include <QStack>

namespace Ui {
class InputDialogFinanceiro;
}

class InputDialogFinanceiro final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { ConfirmarCompra, Financeiro, Historico };
  Q_ENUM(Tipo)

  explicit InputDialogFinanceiro(const Tipo tipo, QWidget *parent);
  ~InputDialogFinanceiro();

  auto getDate() const -> QDate;
  auto getNextDate() const -> QDate;
  auto setFilter(const QString &ordemCompra) -> void;

private:
  // attributes
  bool representacao = false;
  double selectedTotal = 0;
  QStack<int> blockingSignals;
  SqlTableModel modelFluxoCaixa;
  SqlTableModel modelPedidoFornecedor2;
  SqlTableModel modelPedidoFornecedor;
  //  SqlTreeModel modelTree;
  Tipo const tipo;
  Ui::InputDialogFinanceiro *ui;
  // methods
  auto atualizaPrecosPF1(const int rowPF2) -> void;
  auto cadastrar() -> void;
  auto calcularTotal() -> void;
  auto montarFluxoCaixa() -> void;
  auto on_checkBoxMarcarTodos_toggled(const bool checked) -> void;
  auto on_comboBoxST_currentTextChanged() -> void;
  auto on_dateEditEvento_dateChanged(const QDate date) -> void;
  auto on_doubleSpinBoxAliquota_valueChanged(const double aliquota) -> void;
  auto on_doubleSpinBoxFrete_valueChanged() -> void;
  auto on_doubleSpinBoxSt_valueChanged(const double valueSt) -> void;
  auto on_pushButtonCorrigirFluxo_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_table_doubleClicked(const QModelIndex &index) -> void;
  auto on_table_selectionChanged() -> void;
  auto processarPagamento(Pagamento *pgt) -> void;
  auto setCodFornecedor() -> void;
  auto setConnections() -> void;
  auto setMaximumST() -> void;
  //  auto setTreeView() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
  auto verifyFields() -> void;
};
