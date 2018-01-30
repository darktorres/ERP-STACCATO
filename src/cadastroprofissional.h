#ifndef CADASTROPROFISSIONAL_H
#define CADASTROPROFISSIONAL_H

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroProfissional;
}

class CadastroProfissional final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroProfissional(QWidget *parent = nullptr);
  ~CadastroProfissional();

private slots:
  void on_checkBoxMostrarInativos_clicked(const bool checked);
  void on_lineEditCEP_textChanged(const QString &cep);
  void on_lineEditCNPJ_editingFinished();
  void on_lineEditCNPJ_textEdited(const QString &text);
  void on_lineEditCNPJBancario_textEdited(const QString &text);
  void on_lineEditContatoCPF_textEdited(const QString &text);
  void on_lineEditCPF_editingFinished();
  void on_lineEditCPF_textEdited(const QString &text);
  void on_lineEditCPFBancario_textEdited(const QString &text);
  void on_lineEditProfissional_editingFinished();
  void on_pushButtonAdicionarEnd_clicked();
  void on_pushButtonAtualizar_clicked();
  void on_pushButtonAtualizarEnd_clicked();
  void on_pushButtonBuscar_clicked();
  void on_pushButtonCadastrar_clicked();
  void on_pushButtonCancelar_clicked();
  void on_pushButtonEndLimpar_clicked();
  void on_pushButtonNovoCad_clicked();
  void on_pushButtonRemover_clicked();
  void on_pushButtonRemoverEnd_clicked();
  void on_radioButtonPF_toggled(const bool checked);
  void on_tableEndereco_clicked(const QModelIndex &index);
  void on_tableEndereco_entered(const QModelIndex &);

private:
  // attributes
  QString tipoPFPJ;
  SearchDialog *sdProfissional;
  Ui::CadastroProfissional *ui;
  // methods
  auto cadastrar() -> bool final;
  auto cadastrarEndereco(const bool isUpdate = false) -> bool;
  auto savingProcedures() -> bool final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
  auto clearEndereco() -> void;
  auto clearFields() -> void final;
  auto novoEndereco() -> void;
  auto registerMode() -> void final;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto setupUi() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
};

#endif // CADASTROPROFISSIONAL_H
