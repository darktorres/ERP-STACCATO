#pragma once

#include "registerdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroUsuario;
}

class CadastroUsuario final : public RegisterDialog {
  Q_OBJECT

public:
  explicit CadastroUsuario(QWidget *parent);
  ~CadastroUsuario();

  auto modificarUsuario() -> void;

private:
  // attributes
  bool limitado = false;
  SearchDialog *const sdUsuario = SearchDialog::usuario(this);
  SqlTableModel modelPermissoes;
  Ui::CadastroUsuario *ui;
  // methods
  auto cadastrar() -> void final;
  auto clearFields() -> void final;
  auto criarUsuarioMySQL() -> void;
  auto fillComboBoxLoja() -> void;
  auto newRegister() -> bool final;
  auto on_comboBoxTipo_currentTextChanged(const QString &text) -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonDesativar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> void final;
  auto setConnections() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verificaUsuarioDisponivel() -> void;
  auto verifyFields() -> void final;
  auto viewRegister() -> bool final;
};
