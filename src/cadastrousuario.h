#ifndef CADASTRARUSUARIO_H
#define CADASTRARUSUARIO_H

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

private slots:
  void fillCombobox();
  void on_lineEditUser_textEdited(const QString &text);
  void on_pushButtonAtualizar_clicked();
  void on_pushButtonBuscar_clicked();
  void on_pushButtonCadastrar_clicked();
  void on_pushButtonNovoCad_clicked();
  void on_pushButtonRemover_clicked();

private:
  // attributes
  SearchDialog *sdUsuario;
  SqlRelationalTableModel modelPermissoes;
  Ui::CadastroUsuario *ui;
  // methods
  auto cadastrar() -> bool final;
  auto savingProcedures() -> bool final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
  auto clearFields() -> void final;
  auto registerMode() -> void final;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
};

#endif // CADASTRARUSUARIO_H
