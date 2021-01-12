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
  SearchDialog *sdUsuario;
  SqlTableModel modelPermissoes;
  Ui::CadastroUsuario *ui;
  // methods
  auto cadastrar() -> void final;
  auto clearFields() -> void final;
  auto criarUsuarioMySQL() -> void;
  auto fillCombobox() -> void;
  auto newRegister() -> bool final;
  auto on_comboBoxTipo_currentTextChanged(const QString &text) -> void;
  auto on_lineEditUser_textEdited(const QString &text) -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> void final;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verifyFields() -> void final;
  auto viewRegister() -> bool final;
};
