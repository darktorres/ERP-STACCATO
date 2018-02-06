#ifndef INPUTDIALOGFINANCEIRO_H
#define INPUTDIALOGFINANCEIRO_H

#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QLineEdit>

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class InputDialogFinanceiro;
}

class InputDialogFinanceiro final : public Dialog {
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
  SqlRelationalTableModel model;
  SqlRelationalTableModel modelFluxoCaixa;
  Ui::InputDialogFinanceiro *ui;
  // methods
  auto cadastrar() -> bool;
  auto calcularTotal() -> void;
  auto montarFluxoCaixa(const bool updateDate = true) -> void;
  auto on_checkBoxMarcarTodos_toggled(bool checked) -> void;
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text) -> void;
  auto on_comboBoxST_currentTextChanged(const QString &text) -> void;
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_dateEditPgtSt_dateChanged(const QDate &) -> void;
  auto on_doubleSpinBoxAdicionais_valueChanged(const double value) -> void;
  auto on_doubleSpinBoxAliquota_valueChanged(double aliquota) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(double) -> void;
  auto on_doubleSpinBoxPgt_valueChanged() -> void;
  auto on_doubleSpinBoxSt_valueChanged(double valueSt) -> void;
  auto on_doubleSpinBoxTotalPag_valueChanged(double) -> void;
  auto on_pushButtonAdicionarPagamento_clicked() -> void;
  auto on_pushButtonCorrigirFluxo_clicked() -> void;
  auto on_pushButtonLimparPag_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto resetarPagamentos() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
  auto verifyFields() -> bool;
};

#endif // INPUTDIALOGFINANCEIRO_H
