#pragma once

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroProfissional;
}

class CadastroProfissional final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroProfissional(QWidget *parent);
  ~CadastroProfissional();

private:
  // attributes
  QString tipoPFPJ;
  SearchDialog *const sdProfissional = SearchDialog::profissional(false, this);
  Ui::CadastroProfissional *ui;
  // methods
  auto cadastrar() -> void final;
  auto cadastrarEndereco(const Tipo tipoEndereco = Tipo::Cadastrar) -> void;
  auto clearEndereco() -> void;
  auto clearFields() -> void final;
  auto connectLineEditsToDirty() -> void final;
  auto newRegister() -> bool override;
  auto novoEndereco() -> void;
  auto on_checkBoxDataNasc_stateChanged(const int state) -> void;
  auto on_checkBoxMostrarInativos_clicked(const bool checked) -> void;
  auto on_lineEditCEP_textChanged(const QString &cep) -> void;
  auto on_lineEditCNPJBancario_textEdited(const QString &text) -> void;
  auto on_lineEditCNPJ_textEdited(const QString &text) -> void;
  auto on_lineEditCPFBancario_textEdited(const QString &text) -> void;
  auto on_lineEditCPF_textEdited(const QString &text) -> void;
  auto on_lineEditContatoCPF_textEdited(const QString &text) -> void;
  auto on_pushButtonAdicionarEnd_clicked() -> void;
  auto on_pushButtonAtualizarEnd_clicked() -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonDesativarEnd_clicked() -> void;
  auto on_pushButtonDesativar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_radioButtonPF_toggled(const bool checked) -> void;
  auto on_tableEndereco_selectionChanged() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> void final;
  auto setConnections() -> void;
  auto setItemBoxes() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto setupUi() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verificaEndereco() -> void;
  auto verificaVinculo() -> bool;
  auto verifyFields() -> void final;
  auto viewRegister() -> bool final;
};
