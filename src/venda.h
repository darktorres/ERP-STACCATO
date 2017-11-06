#ifndef VENDA_H
#define VENDA_H

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDoubleSpinBox>

#include "registerdialog.h"

namespace Ui {
class Venda;
}

class Venda : public RegisterDialog {
  Q_OBJECT

public:
  explicit Venda(QWidget *parent = 0);
  ~Venda();
  void prepararVenda(const QString &idOrcamento);
  void setFinanceiro();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_doubleSpinBoxTotalPag_valueChanged(double);
  void on_pushButtonAdicionarPagamento_clicked();

private:
  // attributes
  bool financeiro = false;
  double minimoFrete;
  double porcFrete;
  QString idOrcamento;
  SqlRelationalTableModel modelFluxoCaixa;
  SqlRelationalTableModel modelFluxoCaixa2;
  SqlRelationalTableModel modelItem;
  Ui::Venda *ui;
  // methods
  bool atualizarCredito();
  bool cancelamento();
  bool financeiroSalvar();
  bool generateId();
  virtual bool cadastrar() override;
  virtual bool save() override;
  virtual bool savingProcedures() override;
  virtual bool verifyFields() override;
  virtual bool viewRegister() override;
  virtual void clearFields() override;
  virtual void registerMode() override;
  virtual void setupMapper() override;
  virtual void successMessage() override;
  virtual void updateMode() override;
  void calcPrecoGlobalTotal();
  void montarFluxoCaixa();
  void on_checkBoxFreteManual_clicked(const bool checked);
  void on_checkBoxPontuacaoIsento_toggled(bool checked);
  void on_checkBoxPontuacaoPadrao_toggled(bool checked);
  void on_checkBoxRT_toggled(bool checked);
  void on_comboBoxPgt_currentTextChanged(const int index, const QString &text);
  void on_dateTimeEdit_dateTimeChanged(const QDateTime &);
  void on_doubleSpinBoxDescontoGlobal_valueChanged(const double desconto);
  void on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double desconto);
  void on_doubleSpinBoxFrete_valueChanged(const double frete);
  void on_doubleSpinBoxPgt_valueChanged();
  void on_doubleSpinBoxTotal_valueChanged(const double total);
  void on_itemBoxProfissional_textChanged(const QString &);
  void on_pushButtonCadastrarPedido_clicked();
  void on_pushButtonCancelamento_clicked();
  void on_pushButtonCorrigirFluxo_clicked();
  void on_pushButtonDevolucao_clicked();
  void on_pushButtonFinanceiroSalvar_clicked();
  void on_pushButtonFreteLoja_clicked();
  void on_pushButtonGerarExcel_clicked();
  void on_pushButtonImprimir_clicked();
  void on_pushButtonLimparPag_clicked();
  void on_pushButtonPgtLoja_clicked();
  void on_pushButtonVoltar_clicked();
  void on_tableFluxoCaixa_entered(const QModelIndex &);
  void on_tableFluxoCaixa2_entered(const QModelIndex &);
  void resetarPagamentos();
  void setupConnections();
  void setupTables();
  void unsetConnections();
};

#endif // VENDA_H
