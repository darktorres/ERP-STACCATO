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
  explicit CadastroCliente(QWidget *parent = 0);
  ~CadastroCliente();

private slots:
  void on_checkBoxInscEstIsento_toggled(bool checked);
  void on_checkBoxMostrarInativos_clicked(const bool checked);
  void on_lineEditCEP_textChanged(const QString &cep);
  void on_lineEditCNPJ_textEdited(const QString &text);
  void on_lineEditContatoCPF_textEdited(const QString &text);
  void on_lineEditCPF_textEdited(const QString &text);
  void on_pushButtonAdicionarEnd_clicked();
  void on_pushButtonAtualizar_clicked();
  void on_pushButtonAtualizarEnd_clicked();
  void on_pushButtonBuscar_clicked();
  void on_pushButtonCadastrar_clicked();
  void on_pushButtonEndLimpar_clicked();
  void on_pushButtonNovoCad_clicked();
  void on_pushButtonRemover_clicked();
  void on_pushButtonRemoverEnd_clicked();
  void on_radioButtonPF_toggled(const bool checked);
  void on_tableEndereco_clicked(const QModelIndex &index);
  void on_tableEndereco_entered(const QModelIndex &);

private:
  // attributes
  bool incompleto = false;
  QString tipoPFPJ;
  SearchDialog *sdCliente;
  Ui::CadastroCliente *ui;
  // methods
  bool cadastrar() final;
  bool cadastrarEndereco(const Tipo tipo = Tipo::Cadastrar);
  bool savingProcedures() final;
  bool verifyFields() final;
  bool viewRegister() final;
  void clearEndereco();
  void clearFields() final;
  void novoEndereco();
  void registerMode() final;
  void setupMapper() final;
  void setupTables();
  void setupUi();
  void successMessage() final;
  void updateMode() final;
};

#endif // CADASTROCLIENTE_H
