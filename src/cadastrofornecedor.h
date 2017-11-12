#ifndef CADASTROFORNECEDOR_H
#define CADASTROFORNECEDOR_H

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroFornecedor;
}

class CadastroFornecedor final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroFornecedor(QWidget *parent = 0);
  ~CadastroFornecedor();

private slots:
  void on_lineEditCEP_textChanged(const QString &cep);
  void on_lineEditCNPJ_textEdited(const QString &text);
  void on_lineEditContatoCPF_textEdited(const QString &text);
  void on_pushButtonAdicionarEnd_clicked();
  void on_pushButtonAtualizar_clicked();
  void on_pushButtonAtualizarEnd_clicked();
  void on_pushButtonBuscar_clicked();
  void on_pushButtonCadastrar_clicked();
  void on_pushButtonNovoCad_clicked();
  void on_pushButtonRemover_clicked();
  void on_pushButtonRemoverEnd_clicked();
  void on_pushButtonValidade_clicked();
  void on_tableEndereco_clicked(const QModelIndex &index);
  void on_tableEndereco_entered(const QModelIndex &);

private:
  // attributes
  SearchDialog *sdFornecedor;
  Ui::CadastroFornecedor *ui;
  // methods
  bool ajustarValidade(const int novaValidade);
  bool cadastrar() final;
  bool cadastrarEndereco(const bool isUpdate = false);
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

#endif // CADASTROFORNECEDOR_H
