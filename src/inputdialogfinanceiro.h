#ifndef INPUTDIALOGFINANCEIRO_H
#define INPUTDIALOGFINANCEIRO_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class InputDialogFinanceiro;
}

class InputDialogFinanceiro final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { ConfirmarCompra, Financeiro };

  explicit InputDialogFinanceiro(const Tipo &tipo, QWidget *parent = nullptr);
  ~InputDialogFinanceiro();
  auto getDate() const -> QDateTime;
  auto getNextDate() const -> QDateTime;
  auto setFilter(const QString &idCompra) -> bool;

private:
  // attributes
  bool representacao;
  const Tipo tipo;
  SqlRelationalTableModel modelPedidoFornecedor;
  SqlRelationalTableModel modelFluxoCaixa;
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
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
  auto verifyFields() -> bool;
};

#endif // INPUTDIALOGFINANCEIRO_H
