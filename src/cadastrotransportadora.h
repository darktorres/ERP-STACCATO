#pragma once

#include "registeraddressdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroTransportadora;
}

class CadastroTransportadora final : public RegisterAddressDialog {
  Q_OBJECT

public:
  explicit CadastroTransportadora(QWidget *parent);
  ~CadastroTransportadora();

private:
  // attributes
  int currentRowVeiculo = -1;
  QDataWidgetMapper mapperVeiculo;
  QList<QSqlRecord> backupVeiculo;
  SearchDialog *const sdTransportadora = SearchDialog::transportadora(this);
  SqlTableModel modelVeiculo;
  Ui::CadastroTransportadora *ui;
  // methods
  auto cadastrar() -> void final;
  auto cadastrarEndereco(const Tipo tipoEndereco = Tipo::Cadastrar) -> void;
  auto cadastrarVeiculo(const Tipo tipoVeiculo = Tipo::Cadastrar) -> void;
  auto clearEndereco() -> void;
  auto clearFields() -> void final;
  auto clearVeiculo() -> void;
  auto connectLineEditsToDirty() -> void final;
  auto newRegister() -> bool final;
  auto novoEndereco() -> void;
  auto novoVeiculo() -> void;
  auto on_checkBoxMostrarInativosVeiculo_toggled(const bool checked) -> void;
  auto on_checkBoxMostrarInativos_clicked(const bool checked) -> void;
  auto on_lineEditCEP_textChanged(const QString &cep) -> void;
  auto on_lineEditCNPJ_textEdited(const QString &text) -> void;
  auto on_pushButtonAdicionarEnd_clicked() -> void;
  auto on_pushButtonAdicionarVeiculo_clicked() -> void;
  auto on_pushButtonAtualizarEnd_clicked() -> void;
  auto on_pushButtonAtualizarVeiculo_clicked() -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonBuscar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonDesativarEnd_clicked() -> void;
  auto on_pushButtonDesativarVeiculo_clicked() -> void;
  auto on_pushButtonDesativar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_tableEndereco_selectionChanged() -> void;
  auto on_tableVeiculo_selectionChanged() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> void final;
  auto setConnections() -> void;
  auto setupMapper() -> void final;
  auto setupTables() -> void;
  auto setupUi() -> void;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verificaEndereco() -> void;
  auto verifyFields() -> void final;
  auto verifyFieldsVeiculo() -> void;
  auto viewRegister() -> bool final;
};
