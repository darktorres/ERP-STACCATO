#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QStack>

namespace Ui {
class InputDialogProduto;
}

class InputDialogProduto final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { GerarCompra, ConfirmarCompra, Faturamento };
  Q_ENUM(Tipo)

  explicit InputDialogProduto(const Tipo tipo, QWidget *parent);
  ~InputDialogProduto();

  auto getDate() const -> QDate;
  auto getNextDate() const -> QDate;
  auto setFilter(const QStringList &ids) -> void;

private:
  // attributes
  QStack<int> blockingSignals;
  SqlTableModel modelPedidoFornecedor;
  Tipo const tipo;
  Ui::InputDialogProduto *ui;
  // methods
  auto cadastrar() -> void;
  auto calcularTotal() -> void;
  auto on_comboBoxST_currentTextChanged(const QString &text) -> void;
  auto on_dateEditEvento_dateChanged(const QDate date) -> void;
  auto on_doubleSpinBoxAliquota_valueChanged(const double aliquota) -> void;
  auto on_doubleSpinBoxDescontoGlobal_valueChanged(const double value) -> void;
  auto on_doubleSpinBoxST_valueChanged(const double valueSt) -> void;
  auto on_lineEditCodRep_textEdited(const QString &text) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
};
