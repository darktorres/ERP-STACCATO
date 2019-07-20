#ifndef ORCAMENTO_H
#define ORCAMENTO_H

#include "registerdialog.h"

namespace Ui {
class Orcamento;
}

class Orcamento final : public RegisterDialog {
  Q_OBJECT

public:
  explicit Orcamento(QWidget *parent = nullptr);
  ~Orcamento();
  auto show() -> void;

private:
  // attributes
  QList<QSqlRecord> backupItem;
  int currentRowItem = -1;
  bool isReadOnly = false;
  int currentItemIsEstoque = 0;
  bool currentItemIsPromocao = false;
  double minimoFrete = 0;
  double porcFrete = 0;
  QDataWidgetMapper mapperItem;
  SqlRelationalTableModel modelItem;
  Ui::Orcamento *ui;
  // methods
  auto adicionarItem(const Tipo tipoItem = Tipo::Cadastrar) -> void;
  auto atualizaReplica() -> bool;
  auto atualizarItem() -> void;
  auto buscarCadastrarConsultor() -> bool;
  auto buscarParametrosFrete() -> bool;
  auto cadastrar() -> bool final;
  auto calcPrecoGlobalTotal() -> void;
  auto clearFields() -> void final;
  auto generateId() -> bool;
  auto newRegister() -> bool final;
  auto novoItem() -> void;
  auto on_checkBoxFreteManual_clicked(const bool checked) -> void;
  auto on_checkBoxRepresentacao_toggled(const bool checked) -> void;
  auto on_dataEmissao_dateChanged(const QDate &date) -> void;
  auto on_doubleSpinBoxCaixas_valueChanged(const double caixas) -> void;
  auto on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double desconto) -> void;
  auto on_doubleSpinBoxDescontoGlobal_valueChanged(const double desconto) -> void;
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
  auto on_pushButtonGerarVenda_clicked() -> void;
  auto on_pushButtonImprimir_clicked() -> void;
  auto on_pushButtonRemoverItem_clicked() -> void;
  auto on_pushButtonReplicar_clicked() -> void;
  auto on_tableProdutos_clicked(const QModelIndex &index) -> void;
  auto recalcularTotais() -> bool;
  auto registerMode() -> void final;
  auto removeItem() -> void;
  auto savingProcedures() -> bool final;
  auto setConnections() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto unsetConnections() -> void;
  auto updateMode() -> void final;
  auto verificaCadastroCliente() -> bool;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
};

#endif // ORCAMENTO_H
