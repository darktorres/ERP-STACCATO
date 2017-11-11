#ifndef CADASTROTRANSPORTADORA_H
#define CADASTROTRANSPORTADORA_H

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroTransportadora;
}

class CadastroTransportadora final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroTransportadora(QWidget *parent = 0);
  ~CadastroTransportadora();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_checkBoxMostrarInativos_clicked(const bool checked);
  void on_checkBoxMostrarInativosVeiculo_toggled(bool checked);
  void on_lineEditCEP_textChanged(const QString &cep);
  void on_lineEditCNPJ_textEdited(const QString &text);
  void on_pushButtonAdicionarEnd_clicked();
  void on_pushButtonAdicionarVeiculo_clicked();
  void on_pushButtonAtualizar_clicked();
  void on_pushButtonAtualizarEnd_clicked();
  void on_pushButtonAtualizarVeiculo_clicked();
  void on_pushButtonBuscar_clicked();
  void on_pushButtonCadastrar_clicked();
  void on_pushButtonEndLimpar_clicked();
  void on_pushButtonNovoCad_clicked();
  void on_pushButtonRemover_clicked();
  void on_pushButtonRemoverEnd_clicked();
  void on_pushButtonRemoverVeiculo_clicked();
  void on_pushButtonVeiculoLimpar_clicked();
  void on_tableEndereco_clicked(const QModelIndex &index);
  void on_tableEndereco_entered(const QModelIndex &);
  void on_tableVeiculo_clicked(const QModelIndex &index);
  void on_tableVeiculo_entered(const QModelIndex &);

private:
  // attributes
  QDataWidgetMapper mapperVeiculo;
  SearchDialog *sdTransportadora;
  SqlRelationalTableModel modelVeiculo;
  Ui::CadastroTransportadora *ui;
  // methods
  bool cadastrar() final;
  bool cadastrarEndereco(const bool isUpdate = false);
  bool cadastrarVeiculo(const bool isUpdate = false);
  bool save() final;
  bool savingProcedures() final;
  bool verifyFields() final;
  bool viewRegister() final;
  void clearEndereco();
  void clearFields() final;
  void clearVeiculo();
  void novoEndereco();
  void novoVeiculo();
  void registerMode() final;
  void setupMapper() final;
  void setupTables();
  void setupUi();
  void successMessage() final;
  void updateMode() final;
};

#endif // CADASTROTRANSPORTADORA_H
