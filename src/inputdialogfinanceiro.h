#ifndef INPUTDIALOGFINANCEIRO_H
#define INPUTDIALOGFINANCEIRO_H

#include <QComboBox>
#include <QDateEdit>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QLineEdit>

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
  QDateTime getDate() const;
  QDateTime getNextDate() const;
  bool setFilter(const QString &idCompra);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private:
  // attributes
  bool representacao;
  const Tipo tipo;
  SqlRelationalTableModel model;
  SqlRelationalTableModel modelFluxoCaixa;
  Ui::InputDialogFinanceiro *ui;
  // methods
  bool cadastrar();
  bool verifyFields();
  void calcularTotal();
  void montarFluxoCaixa(const bool updateDate = true);
  void on_checkBoxMarcarTodos_toggled(bool checked);
  void on_comboBoxPgt_currentTextChanged(const int index, const QString &text);
  void on_dateEditEvento_dateChanged(const QDate &date);
  void on_dateEditPgtSt_dateChanged(const QDate &);
  void on_doubleSpinBoxAdicionais_valueChanged(const double value);
  void on_doubleSpinBoxFrete_valueChanged(double);
  void on_doubleSpinBoxPgt_valueChanged();
  void on_doubleSpinBoxSt_valueChanged(double);
  void on_doubleSpinBoxTotalPag_valueChanged(double);
  void on_pushButtonAdicionarPagamento_clicked();
  void on_pushButtonCorrigirFluxo_clicked();
  void on_pushButtonLimparPag_clicked();
  void on_pushButtonSalvar_clicked();
  void resetarPagamentos();
  void setConnections();
  void setupTables();
  void updateTableData(const QModelIndex &topLeft);
  void unsetConnections();
};

#endif // INPUTDIALOGFINANCEIRO_H
