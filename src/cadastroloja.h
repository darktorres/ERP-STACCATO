#ifndef CADASTROLOJA_H
#define CADASTROLOJA_H

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroLoja;
}

class CadastroLoja final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroLoja(QWidget *parent = nullptr);
  ~CadastroLoja();

private:
  // attributes
  QDataWidgetMapper mapperConta;
  QDataWidgetMapper mapperPagamento;
  SearchDialog *sdLoja;
  SqlRelationalTableModel modelAssocia1;
  SqlRelationalTableModel modelAssocia2;
  SqlRelationalTableModel modelConta;
  SqlRelationalTableModel modelPagamentos;
  SqlRelationalTableModel modelPermissoes;
  SqlRelationalTableModel modelTaxas;
  Ui::CadastroLoja *ui;
  // methods
  bool adicionarPagamento();
  bool atualizarPagamento();
  bool cadastrar() final;
  bool cadastrarConta(const bool isUpdate = false);
  bool cadastrarEndereco(const bool isUpdate = false);
  bool newRegister() final;
  bool savingProcedures() final;
  bool verifyFields() final;
  bool viewRegister() final;
  auto clearConta() -> void;
  auto clearEndereco() -> void;
  auto clearFields() -> void final;
  auto novaConta() -> void;
  auto novoEndereco() -> void;
  auto registerMode() -> void final;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto setupUi() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto on_checkBoxMostrarInativos_clicked(const bool checked) -> void;
  auto on_checkBoxMostrarInativosConta_clicked(bool checked) -> void;
  auto on_lineEditCEP_textChanged(const QString &cep) -> void;
  auto on_lineEditCNPJ_textEdited(const QString &text) -> void;
  auto on_pushButtonAdicionaAssociacao_clicked() -> void;
  auto on_pushButtonAdicionarConta_clicked() -> void;
  auto on_pushButtonAdicionarEnd_clicked() -> void;
  auto on_pushButtonAdicionarPagamento_clicked() -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonAtualizarConta_clicked() -> void;
  auto on_pushButtonAtualizarEnd_clicked() -> void;
  auto on_pushButtonAtualizarPagamento_clicked() -> void;
  auto on_pushButtonAtualizarTaxas_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonContaLimpar_clicked() -> void;
  auto on_pushButtonEndLimpar_clicked() -> void;
  auto on_pushButtonLimparSelecao_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_pushButtonRemoveAssociacao_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto on_pushButtonRemoverConta_clicked() -> void;
  auto on_pushButtonRemoverEnd_clicked() -> void;
  auto on_pushButtonRemoverPagamento_clicked() -> void;
  auto on_tableConta_clicked(const QModelIndex &index) -> void;
  auto on_tableEndereco_clicked(const QModelIndex &index) -> void;
  auto on_tableEndereco_entered(const QModelIndex &) -> void;
  auto on_tablePagamentos_clicked(const QModelIndex &index) -> void;
};

#endif // CADASTROLOJA_H
