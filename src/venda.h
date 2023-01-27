#pragma once

#include "registerdialog.h"
#include "sqltreemodel.h"
#include "widgetpagamentos.h"

#include <QStack>

namespace Ui {
class Venda;
}

class Venda final : public RegisterDialog {
  Q_OBJECT

public:
  explicit Venda(QWidget *parent);
  ~Venda();

  auto prepararVenda(const QString &idOrcamento) -> void;
  auto setFinanceiro() -> void;
  auto show() -> void;

private:
  // attributes
  bool canChangeFrete = false;
  bool correcao = false;
  bool financeiro = false;
  bool representacao = false;
  double minimoFrete = 0;
  double minimoGerente = 0.;
  double porcFrete = 0;
  int idLoja = 0;
  QList<QSqlRecord> backupItem;
  QStack<int> blockingSignals;
  SqlTableModel modelFluxoCaixa2;
  SqlTableModel modelFluxoCaixa;
  SqlTableModel modelItem2;
  SqlTableModel modelItem;
  SqlTreeModel modelTree;
  Ui::Venda *ui;
  // methods
  auto atualizarCredito() -> void;
  auto cadastrar() -> void final;
  auto calcularFrete(const bool updateSpinBox) -> void;
  auto calcularPesoTotal() -> void;
  auto cancelamento() -> void;
  auto clearFields() -> void final;
  auto connectLineEditsToDirty() -> void final;
  auto copiaProdutosOrcamento() -> void;
  auto criarComissaoProfissional() -> void;
  auto criarConsumos() -> void;
  auto financeiroSalvar() -> void;
  auto generateId() -> void;
  auto montarFluxoCaixa() -> void;
  auto on_checkBoxFreteManual_clicked(const bool checked) -> void;
  auto on_checkBoxMostrarCancelados_toggled(const bool checked) -> void;
  auto on_checkBoxPontuacaoIsento_toggled(const bool checked) -> void;
  auto on_checkBoxPontuacaoPadrao_toggled(const bool checked) -> void;
  auto on_checkBoxRT_toggled(const bool checked) -> void;
  auto on_dateTimeEdit_dateTimeChanged() -> void;
  auto on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) -> void;
  auto on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(const double frete) -> void;
  auto on_doubleSpinBoxTotal_valueChanged(const double total) -> void;
  auto on_itemBoxEndereco_idChanged() -> void;
  auto on_itemBoxProfissional_textChanged() -> void;
  auto on_pushButtonAbrirOrcamento_clicked() -> void;
  auto on_pushButtonAdicionarObservacao_clicked() -> void;
  auto on_pushButtonAtualizarEnderecoEnt_clicked() -> void;
  auto on_pushButtonCadastrarVenda_clicked() -> void;
  auto on_pushButtonCancelamento_clicked() -> void;
  auto on_pushButtonComprovantes_clicked() -> void;
  auto on_pushButtonCorrigirFluxo_clicked() -> void;
  auto on_pushButtonDevolucao_clicked() -> void;
  auto on_pushButtonFinanceiroSalvar_clicked() -> void;
  auto on_pushButtonGerarExcel_clicked() -> void;
  auto on_pushButtonGerarPdf_clicked() -> void;
  auto on_pushButtonModelo3d_clicked() -> void;
  auto on_pushButtonVoltar_clicked() -> void;
  auto on_treeView_doubleClicked(const QModelIndex &index) -> void;
  auto processarPagamento(Pagamento *pgt) -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> void final;
  auto setConnections() -> void;
  auto setItemBoxes() -> void;
  auto setTreeView() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto trocarEnderecoEntrega() -> void;
  auto unsetConnections() -> void;
  auto updateMode() -> void final;
  auto verificaDisponibilidadeEstoque() -> void;
  auto verificaFreteLoja() -> void;
  auto verifyFields() -> void final;
  auto viewRegister() -> bool final;
};
