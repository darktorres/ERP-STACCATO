#ifndef INPUTDIALOGPRODUTO_H
#define INPUTDIALOGPRODUTO_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class InputDialogProduto;
}

class InputDialogProduto : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { GerarCompra, Faturamento };

  explicit InputDialogProduto(const Tipo &tipo, QWidget *parent = 0);
  ~InputDialogProduto();
  QDateTime getDate() const;
  QDateTime getNextDate() const;
  bool setFilter(const QStringList &ids);

private slots:
  void on_comboBoxST_currentTextChanged(const QString &);
  void on_dateEditEvento_dateChanged(const QDate &date);
  void on_doubleSpinBoxAliquota_valueChanged(double);
  void on_doubleSpinBoxST_valueChanged(double value);
  void on_pushButtonSalvar_clicked();
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  // REFAC: refactor this out
  bool isBlockedAliquota = false;
  const Tipo tipo;
  SqlRelationalTableModel model;
  Ui::InputDialogProduto *ui;
  // methods
  bool cadastrar();
  void calcularTotal();
  void processST();
  void setupTables();
  void updateTableData(const QModelIndex &topLeft);
};

#endif // INPUTDIALOGPRODUTO_H
