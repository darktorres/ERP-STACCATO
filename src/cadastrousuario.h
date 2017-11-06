#ifndef CADASTRARUSUARIO_H
#define CADASTRARUSUARIO_H

#include "registerdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroUsuario;
}

class CadastroUsuario : public RegisterDialog {
  Q_OBJECT

public:
  explicit CadastroUsuario(QWidget *parent = 0);
  ~CadastroUsuario();
  void modificarUsuario();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

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
  virtual bool cadastrar() override;
  virtual bool save() override;
  virtual bool savingProcedures() override;
  virtual bool verifyFields() override;
  virtual bool viewRegister() override;
  virtual void clearFields() override;
  virtual void registerMode() override;
  virtual void setupMapper() override;
  virtual void successMessage() override;
  virtual void updateMode() override;
  void setupTables();
};

#endif // CADASTRARUSUARIO_H
