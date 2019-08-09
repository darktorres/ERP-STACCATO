#pragma once

#include "registerdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroUsuario;
}

class CadastroUsuario final : public RegisterDialog {
  Q_OBJECT

public:
  explicit CadastroUsuario(QWidget *parent = nullptr);
  ~CadastroUsuario();
  auto modificarUsuario() -> void;

private:
  // attributes
  SearchDialog *sdUsuario;
  SqlRelationalTableModel modelPermissoes;
  Ui::CadastroUsuario *ui;
  // methods
  auto cadastrar() -> bool final;
  auto clearFields() -> void final;
  auto fillCombobox() -> void;
  auto on_comboBoxTipo_currentTextChanged(const QString &text) -> void;
  auto on_lineEditUser_textEdited(const QString &text) -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> bool final;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
};
