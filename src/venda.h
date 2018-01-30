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

class Venda final : public RegisterDialog {
  Q_OBJECT

public:
  explicit Venda(QWidget *parent = nullptr);
  ~Venda();
  void prepararVenda(const QString &idOrcamento);
  void setFinanceiro();

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
  bool atualizaQuantEstoque();
  bool atualizarCredito();
  bool cadastrar() final;
  bool cancelamento();
  bool financeiroSalvar();
  bool generateId();
  bool savingProcedures() final;
  bool todosProdutosSaoEstoque();
  bool verifyFields() final;
  bool viewRegister() final;
  void calcPrecoGlobalTotal();
  void clearFields() final;
  void montarFluxoCaixa();
  void on_checkBoxFreteManual_clicked(const bool checked);
  void on_checkBoxPontuacaoIsento_toggled(bool checked);
  void on_checkBoxPontuacaoPadrao_toggled(bool checked);
  void on_checkBoxRT_toggled(bool checked);
  void on_comboBoxPgt_currentTextChanged(const int index, const QString &text);
  void on_dateTimeEdit_dateTimeChanged(const QDateTime &);
  void on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double desconto);
  void on_doubleSpinBoxDescontoGlobal_valueChanged(const double desconto);
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
  void on_tableFluxoCaixa2_entered(const QModelIndex &);
  void on_tableFluxoCaixa_entered(const QModelIndex &);
  void registerMode() final;
  void resetarPagamentos();
  void setupConnections();
  void setupMapper() final;
  void setupTables();
  void successMessage() final;
  void unsetConnections();
  void updateMode() final;
};

#endif // VENDA_H
