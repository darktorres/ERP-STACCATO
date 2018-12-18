#ifndef CADASTROFORNECEDOR_H
#define CADASTROFORNECEDOR_H

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroFornecedor;
}

class CadastroFornecedor final : public RegisterAddressDialog {
  Q_OBJECT

public:
  enum class Especialidade { Revestimentos = 1, Loucas_Metais = 2, Acessorios = 3, Insumos = 4, Servicos = 5 };
  explicit CadastroFornecedor(QWidget *parent = nullptr);
  ~CadastroFornecedor() final;

private:
  // attributes
  SearchDialog *sdFornecedor;
  Ui::CadastroFornecedor *ui;
  // methods
  auto ajustarValidade(const int novaValidade) -> bool;
  auto cadastrar() -> bool final;
  auto cadastrarEndereco(const CadastroFornecedor::Tipo tipo = Tipo::Cadastrar) -> bool;
  auto clearEndereco() -> void;
  auto clearFields() -> void final;
  auto novoEndereco() -> void;
  auto on_checkBoxMostrarInativos_clicked(const bool checked) -> void;
  auto on_lineEditCEP_textChanged(const QString &cep) -> void;
  auto on_lineEditCNPJ_textEdited(const QString &text) -> void;
  auto on_lineEditContatoCPF_textEdited(const QString &text) -> void;
  auto on_pushButtonAdicionarEnd_clicked() -> void;
  auto on_pushButtonAtualizarEnd_clicked() -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_pushButtonRemoverEnd_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto on_pushButtonValidade_clicked() -> void;
  auto on_tableEndereco_clicked(const QModelIndex &index) -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> bool final;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto setupUi() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
};

#endif // CADASTROFORNECEDOR_H
