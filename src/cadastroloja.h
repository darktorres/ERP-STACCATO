#ifndef CADASTROLOJA_H
#define CADASTROLOJA_H

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroLoja;
}

class CadastroLoja final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroLoja(QWidget *parent = 0);
  ~CadastroLoja();

private slots:
  void on_checkBoxMostrarInativos_clicked(const bool checked);
  void on_checkBoxMostrarInativosConta_clicked(bool checked);
  void on_lineEditCEP_textChanged(const QString &cep);
  void on_lineEditCNPJ_textEdited(const QString &text);
  void on_pushButtonAdicionaAssociacao_clicked();
  void on_pushButtonAdicionarConta_clicked();
  void on_pushButtonAdicionarEnd_clicked();
  void on_pushButtonAdicionarPagamento_clicked();
  void on_pushButtonAtualizar_clicked();
  void on_pushButtonAtualizarConta_clicked();
  void on_pushButtonAtualizarEnd_clicked();
  void on_pushButtonAtualizarPagamento_clicked();
  void on_pushButtonAtualizarTaxas_clicked();
  void on_pushButtonBuscar_clicked();
  void on_pushButtonCadastrar_clicked();
  void on_pushButtonContaLimpar_clicked();
  void on_pushButtonEndLimpar_clicked();
  void on_pushButtonLimparSelecao_clicked();
  void on_pushButtonNovoCad_clicked();
  void on_pushButtonRemoveAssociacao_clicked();
  void on_pushButtonRemover_clicked();
  void on_pushButtonRemoverConta_clicked();
  void on_pushButtonRemoverEnd_clicked();
  void on_pushButtonRemoverPagamento_clicked();
  void on_tableConta_clicked(const QModelIndex &index);
  void on_tableEndereco_clicked(const QModelIndex &index);
  void on_tableEndereco_entered(const QModelIndex &);
  void on_tablePagamentos_clicked(const QModelIndex &index);

private:
  // attributes
  QDataWidgetMapper mapperConta;
  QDataWidgetMapper mapperPagamento;
  SearchDialog *sdLoja;
  SqlRelationalTableModel modelAssocia1;
  SqlRelationalTableModel modelAssocia2;
  SqlRelationalTableModel modelConta;
  SqlRelationalTableModel modelPagamentos;
  SqlRelationalTableModel modelPermissoes;
  SqlRelationalTableModel modelTaxas;
  Ui::CadastroLoja *ui;
  // methods
  bool adicionarPagamento();
  bool atualizarPagamento();
  bool cadastrar() final;
  bool cadastrarConta(const bool isUpdate = false);
  bool cadastrarEndereco(const bool isUpdate = false);
  bool newRegister() final;
  bool savingProcedures() final;
  bool verifyFields() final;
  bool viewRegister() final;
  void clearConta();
  void clearEndereco();
  void clearFields() final;
  void novaConta();
  void novoEndereco();
  void registerMode() final;
  void setupMapper() final;
  void setupTables();
  void setupUi();
  void successMessage() final;
  void updateMode() final;
};

#endif // CADASTROLOJA_H
