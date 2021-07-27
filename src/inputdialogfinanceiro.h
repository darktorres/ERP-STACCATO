#pragma once

#include "sqltablemodel.h"
#include "sqltreemodel.h"

#include <QDialog>
#include <QStack>

namespace Ui {
class InputDialogFinanceiro;
}

class InputDialogFinanceiro final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { ConfirmarCompra, Financeiro };
  Q_ENUM(Tipo)

  explicit InputDialogFinanceiro(const Tipo &tipo, QWidget *parent);
  ~InputDialogFinanceiro();

  auto getDate() const -> QDate;
  auto getNextDate() const -> QDate;
  auto setFilter(const QString &idCompra) -> void;

private:
  // attributes
  bool representacao;
  QStack<int> blockingSignals;
  SqlTableModel modelFluxoCaixa;
  SqlTableModel modelPedidoFornecedor2;
  SqlTableModel modelPedidoFornecedor;
  SqlTreeModel modelTree;
  Tipo const tipo;
  Ui::InputDialogFinanceiro *ui;
  // methods
  auto atualizaPrecosPF1(const int rowPF2) -> void;
  auto cadastrar() -> void;
  auto calcularTotal() -> void;
  auto montarFluxoCaixa(const bool updateDate = true) -> void;
  auto on_checkBoxDataFrete_toggled(bool checked) -> void;
  auto on_checkBoxMarcarTodos_toggled(const bool checked) -> void;
  auto on_checkBoxParcelarSt_toggled(bool) -> void;
  auto on_comboBoxST_currentTextChanged(const QString &text) -> void;
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_dateEditFrete_dateChanged(const QDate &) -> void;
  auto on_dateEditPgtSt_dateChanged(const QDate &) -> void;
  auto on_doubleSpinBoxAliquota_valueChanged(const double aliquota) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(double) -> void;
  auto on_doubleSpinBoxSt_valueChanged(const double valueSt) -> void;
  auto on_lineEditCodFornecedor_textChanged(const QString &text) -> void;
  auto on_pushButtonCorrigirFluxo_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setTreeView() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
  auto verifyFields() -> void;
};
