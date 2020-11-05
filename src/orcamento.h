#pragma once

#include "registerdialog.h"

namespace Ui {
class Orcamento;
}

class Orcamento final : public RegisterDialog {
  Q_OBJECT

public:
  explicit Orcamento(QWidget *parent);
  ~Orcamento();

  auto show() -> void;

private:
  // attributes
  QList<QSqlRecord> backupItem;
  int currentRowItem = -1;
  bool isReadOnly = false;
  bool currentItemIsEstoque = false;
  int currentItemIsPromocao = 0;
  bool canChangeFrete = false;
  double minimoFrete = 0;
  double porcFrete = 0;
  QDataWidgetMapper mapperItem;
  SqlTableModel modelItem;
  Ui::Orcamento *ui;
  // methods
  auto cadastrar() -> void final;
  auto clearFields() -> void final;
  auto newRegister() -> bool final;
  auto registerMode() -> void final;
  auto savingProcedures() -> void final;
  auto setupMapper() -> void final;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verifyFields() -> void final;
  auto viewRegister() -> bool final;

  auto adicionarItem(const Tipo tipoItem = Tipo::Cadastrar) -> void;
  auto atualizaReplica() -> void;
  auto atualizarItem() -> void;
  auto buscarCadastrarConsultor() -> void;
  auto buscarParametrosFrete() -> void;
  auto calcPrecoGlobalTotal() -> void;
  auto generateId() -> void;
  auto novoItem() -> void;
  auto on_checkBoxFreteManual_clicked(const bool checked) -> void;
  auto on_checkBoxRepresentacao_toggled(const bool checked) -> void;
  auto on_dataEmissao_dateChanged(const QDate &date) -> void;
  auto on_doubleSpinBoxCaixas_valueChanged(const double caixas) -> void;
  auto on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) -> void;
  auto on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) -> void;
  auto on_doubleSpinBoxDesconto_valueChanged(const double desconto) -> void;
  auto on_doubleSpinBoxFrete_valueChanged(const double frete) -> void;
  auto on_doubleSpinBoxQuant_valueChanged(const double quant) -> void;
  auto on_doubleSpinBoxTotalItem_valueChanged(const double) -> void;
  auto on_doubleSpinBoxTotal_valueChanged(const double total) -> void;
  auto on_itemBoxCliente_textChanged(const QString &) -> void;
  auto on_itemBoxProduto_idChanged(const QVariant &) -> void;
  auto on_itemBoxVendedor_textChanged(const QString &) -> void;
  auto on_pushButtonAdicionarItem_clicked() -> void;
  auto on_pushButtonApagarOrc_clicked() -> void;
  auto on_pushButtonAtualizarItem_clicked() -> void;
  auto on_pushButtonAtualizarOrcamento_clicked() -> void;
  auto on_pushButtonCadastrarOrcamento_clicked() -> void;
  auto on_pushButtonCalculadora_clicked() -> void;
  auto on_pushButtonCalcularFrete_clicked() -> void;
  auto on_pushButtonGerarExcel_clicked() -> void;
  auto on_pushButtonGerarPdf_clicked() -> void;
  auto on_pushButtonGerarVenda_clicked() -> void;
  auto on_pushButtonModelo3d_clicked() -> void;
  auto on_pushButtonRemoverItem_clicked() -> void;
  auto on_pushButtonReplicar_clicked() -> void;
  auto on_tableProdutos_clicked(const QModelIndex &index) -> void;
  auto recalcularTotais() -> void;
  auto removeItem() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto verificaCadastroCliente() -> void;
  auto verificaDisponibilidadeEstoque() -> void;
};
