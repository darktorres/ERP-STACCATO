#ifndef INPUTDIALOGPRODUTO_H
#define INPUTDIALOGPRODUTO_H

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class InputDialogProduto;
}

class InputDialogProduto final : public Dialog {
  Q_OBJECT

public:
  enum class Tipo { GerarCompra, Faturamento };

  explicit InputDialogProduto(const Tipo &tipo, QWidget *parent = nullptr);
  ~InputDialogProduto();
  auto getDate() const -> QDateTime;
  auto getNextDate() const -> QDateTime;
  auto setFilter(const QStringList &ids) -> bool;

private:
  // attributes
  const Tipo tipo;
  SqlRelationalTableModel model;
  Ui::InputDialogProduto *ui;
  // methods
  auto cadastrar() -> bool;
  auto calcularTotal() -> void;
  auto on_comboBoxST_currentTextChanged(const QString &text) -> void;
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_doubleSpinBoxAliquota_valueChanged(double aliquota) -> void;
  auto on_doubleSpinBoxST_valueChanged(double valueSt) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto updateTableData(const QModelIndex &topLeft) -> void;
};

#endif // INPUTDIALOGPRODUTO_H
