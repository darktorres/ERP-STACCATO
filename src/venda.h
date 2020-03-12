#pragma once

#include "registerdialog.h"
#include "sqltreemodel.h"

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
  QList<QSqlRecord> backupItem;
  bool financeiro = false;
  bool correcao = false;
  int idLoja;
  bool representacao;
  bool canChangeFrete = false;
  double minimoFrete;
  double porcFrete;
  SqlTableModel modelFluxoCaixa;
  SqlTableModel modelFluxoCaixa2;
  SqlTableModel modelItem;
  SqlTableModel modelItem2;
  SqlTreeModel modelTree;
  Ui::Venda *ui;
  // methods
  auto atualizarCredito() -> bool;
  auto cadastrar() -> bool final;
  auto calcPrecoGlobalTotal() -> void;
  auto cancelamento() -> bool;
  auto clearFields() -> void final;
  auto copiaProdutosOrcamento() -> bool;
  auto criarConsumos() -> bool;
  auto financeiroSalvar() -> bool;
  auto generateId() -> bool;
  auto montarFluxoCaixa() -> void;
  auto on_checkBoxFreteManual_clicked(const bool checked) -> void;
  auto on_checkBoxPontuacaoIsento_toggled(bool checked) -> void;
  auto on_checkBoxPontuacaoPadrao_toggled(bool checked) -> void;
  auto on_checkBoxRT_toggled(bool checked) -> void;
  auto on_dateTimeEdit_dateTimeChanged(const QDateTime &) -> void;
  auto on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) -> void;
  auto on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(const double frete) -> void;
  auto on_doubleSpinBoxTotal_valueChanged(const double total) -> void;
  auto on_itemBoxProfissional_textChanged(const QString &) -> void;
  auto on_pushButtonCadastrarPedido_clicked() -> void;
  auto on_pushButtonCancelamento_clicked() -> void;
  auto on_pushButtonComprovantes_clicked() -> void;
  auto on_pushButtonCorrigirFluxo_clicked() -> void;
  auto on_pushButtonDevolucao_clicked() -> void;
  auto on_pushButtonFinanceiroSalvar_clicked() -> void;
  auto on_pushButtonGerarExcel_clicked() -> void;
  auto on_pushButtonGerarPdf_clicked() -> void;
  auto on_pushButtonVoltar_clicked() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> bool final;
  auto setConnections() -> void;
  auto setTreeView() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto unsetConnections() -> void;
  auto updateMode() -> void final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
};
