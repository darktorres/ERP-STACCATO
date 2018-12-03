#ifndef VENDA_H
#define VENDA_H

#include "registerdialog.h"

namespace Ui {
class Venda;
}

class Venda final : public RegisterDialog {
  Q_OBJECT

public:
  explicit Venda(QWidget *parent = nullptr);
  ~Venda();
  auto prepararVenda(const QString &idOrcamento) -> void;
  auto setFinanceiro() -> void;

private:
  // attributes
  bool financeiro = false;
  bool correcao = false;
  int idLoja;
  bool representacao;
  double minimoFrete;
  double porcFrete;
  SqlRelationalTableModel modelFluxoCaixa;
  SqlRelationalTableModel modelFluxoCaixa2;
  SqlRelationalTableModel modelItem;
  Ui::Venda *ui;
  // methods
  auto atualizaQuantEstoque() -> bool;
  auto atualizarCredito() -> bool;
  auto cadastrar() -> bool final;
  auto calcPrecoGlobalTotal() -> void;
  auto cancelamento() -> bool;
  auto clearFields() -> void final;
  auto copiaProdutosOrcamento() -> bool;
  auto financeiroSalvar() -> bool;
  auto generateId() -> bool;
  auto montarFluxoCaixa() -> void;
  auto on_checkBoxFreteManual_clicked(const bool checked) -> void;
  auto on_checkBoxPontuacaoIsento_toggled(bool checked) -> void;
  auto on_checkBoxPontuacaoPadrao_toggled(bool checked) -> void;
  auto on_checkBoxRT_toggled(bool checked) -> void;
  auto on_comboBoxPgt_currentTextChanged(const int index, const QString &text) -> void;
  auto on_dateTimeEdit_dateTimeChanged(const QDateTime &) -> void;
  auto on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double desconto) -> void;
  auto on_doubleSpinBoxDescontoGlobal_valueChanged(const double desconto) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(const double frete) -> void;
  auto on_doubleSpinBoxPgt_valueChanged() -> void;
  auto on_doubleSpinBoxTotalPag_valueChanged(double) -> void;
  auto on_doubleSpinBoxTotal_valueChanged(const double total) -> void;
  auto on_itemBoxProfissional_textChanged(const QString &) -> void;
  auto on_pushButtonAdicionarPagamento_clicked() -> void;
  auto on_pushButtonCadastrarPedido_clicked() -> void;
  auto on_pushButtonCancelamento_clicked() -> void;
  auto on_pushButtonCorrigirFluxo_clicked() -> void;
  auto on_pushButtonDevolucao_clicked() -> void;
  auto on_pushButtonFinanceiroSalvar_clicked() -> void;
  auto on_pushButtonFreteLoja_clicked() -> void;
  auto on_pushButtonGerarExcel_clicked() -> void;
  auto on_pushButtonImprimir_clicked() -> void;
  auto on_pushButtonLimparPag_clicked() -> void;
  auto on_pushButtonPgtLoja_clicked() -> void;
  auto on_pushButtonVoltar_clicked() -> void;
  auto registerMode() -> void final;
  auto resetarPagamentos() -> void;
  auto savingProcedures() -> bool final;
  auto setConnections() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto todosProdutosSaoEstoque() -> bool;
  auto unsetConnections() -> void;
  auto updateMode() -> void final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
};

#endif // VENDA_H
