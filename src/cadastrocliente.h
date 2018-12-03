#ifndef CADASTROCLIENTE_H
#define CADASTROCLIENTE_H

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroCliente;
}

class CadastroCliente final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroCliente(QWidget *parent = nullptr);
  ~CadastroCliente() final;

private:
  // attributes
  QString tipoPFPJ;
  SearchDialog *sdCliente;
  Ui::CadastroCliente *ui;
  // methods
  auto cadastrar() -> bool final;
  auto cadastrarEndereco(const Tipo tipo = Tipo::Cadastrar) -> bool;
  auto clearEndereco() -> void;
  auto clearFields() -> void final;
  auto novoEndereco() -> void;
  auto on_checkBoxInscEstIsento_toggled(bool checked) -> void;
  auto on_checkBoxMostrarInativos_clicked(const bool checked) -> void;
  auto on_lineEditCEP_textChanged(const QString &cep) -> void;
  auto on_lineEditCNPJ_textEdited(const QString &text) -> void;
  auto on_lineEditCPF_textEdited(const QString &text) -> void;
  auto on_lineEditContatoCPF_textEdited(const QString &text) -> void;
  auto on_pushButtonAdicionarEnd_clicked() -> void;
  auto on_pushButtonAtualizarEnd_clicked() -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonEndLimpar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_pushButtonRemoverEnd_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto on_radioButtonPF_toggled(const bool checked) -> void;
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
  auto setConnections() -> void;
};

#endif // CADASTROCLIENTE_H
