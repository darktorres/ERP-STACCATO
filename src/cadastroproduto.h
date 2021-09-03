#pragma once

#include "registerdialog.h"
#include "searchdialog.h"

#include <QStack>

namespace Ui {
class CadastroProduto;
}

class CadastroProduto final : public RegisterDialog {
  Q_OBJECT

public:
  explicit CadastroProduto(QWidget *parent);
  ~CadastroProduto();

private:
  // attributes
  QStack<int> blockingSignals;
  SearchDialog *const sdProduto = SearchDialog::produto(true, true, true, false, this);
  Ui::CadastroProduto *ui;
  // methods
  auto cadastrar() -> void final;
  auto calcularMarkup() -> void;
  auto clearFields() -> void final;
  auto connectLineEditsToDirty() -> void final;
  auto on_checkBoxValidade_stateChanged(const int state) -> void;
  auto on_doubleSpinBoxCusto_valueChanged() -> void;
  auto on_doubleSpinBoxVenda_valueChanged() -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonDesativar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> void final;
  auto setConnections() -> void;
  auto setupMapper() -> void final;
  auto setupUi() -> void;
  auto successMessage() -> void final;
  auto unsetConnections() -> void;
  auto updateMode() -> void final;
  auto verifyFields() -> void final;
  auto viewRegister() -> bool final;
};
