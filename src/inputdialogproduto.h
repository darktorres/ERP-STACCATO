#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class InputDialogProduto;
}

class InputDialogProduto final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { GerarCompra, Faturamento };

  explicit InputDialogProduto(const Tipo &tipo, QWidget *parent);
  ~InputDialogProduto();

  auto getDate() const -> QDate;
  auto getNextDate() const -> QDate;
  auto setFilter(const QStringList &ids) -> void;

private:
  // attributes
  SqlTableModel modelPedidoFornecedor;
  Tipo const tipo;
  Ui::InputDialogProduto *ui;
  // methods
  auto cadastrar() -> void;
  auto calcularTotal() -> void;
  auto on_comboBoxST_currentTextChanged(const QString &text) -> void;
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_doubleSpinBoxAliquota_valueChanged(double aliquota) -> void;
  auto on_doubleSpinBoxST_valueChanged(double valueSt) -> void;
  auto on_lineEditCodRep_textEdited(const QString &text) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
};
