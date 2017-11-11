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
  explicit CadastroUsuario(QWidget *parent = 0);
  ~CadastroUsuario();
  void modificarUsuario();

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
  bool cadastrar() final;
  bool savingProcedures() final;
  bool verifyFields() final;
  bool viewRegister() final;
  void clearFields() final;
  void registerMode() final;
  void setupMapper() final;
  void setupTables();
  void successMessage() final;
  void updateMode() final;
};

#endif // CADASTRARUSUARIO_H
