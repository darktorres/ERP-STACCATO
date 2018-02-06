#ifndef CADASTROPRODUTO_H
#define CADASTROPRODUTO_H

#include "registerdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroProduto;
}

class CadastroProduto final : public RegisterDialog {
  Q_OBJECT

public:
  explicit CadastroProduto(QWidget *parent = nullptr);
  ~CadastroProduto();

private:
  // attributes
  SearchDialog *sdProduto;
  Ui::CadastroProduto *ui;
  // methods
  auto cadastrar() -> bool final;
  auto calcularMarkup() -> void;
  auto clearFields() -> void final;
  auto on_doubleSpinBoxCusto_valueChanged(const double &) -> void;
  auto on_doubleSpinBoxVenda_valueChanged(const double &) -> void;
  auto on_pushButtonAtualizar_clicked() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonNovoCad_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto registerMode() -> void final;
  auto savingProcedures() -> bool final;
  auto setupMapper() -> void final;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
};

#endif // CADASTROPRODUTO_H
