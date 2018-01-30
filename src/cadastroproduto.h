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

private slots:
  void on_doubleSpinBoxCusto_valueChanged(const double &);
  void on_doubleSpinBoxVenda_valueChanged(const double &);
  void on_pushButtonAtualizar_clicked();
  void on_pushButtonCadastrar_clicked();
  void on_pushButtonNovoCad_clicked();
  void on_pushButtonRemover_clicked();

private:
  // attributes
  SearchDialog *sdProduto;
  Ui::CadastroProduto *ui;
  // methods
  auto cadastrar() -> bool final;
  auto savingProcedures() -> bool final;
  auto verifyFields() -> bool final;
  auto viewRegister() -> bool final;
  auto calcularMarkup() -> void;
  auto clearFields() -> void final;
  auto registerMode() -> void final;
  auto setupMapper() -> void final;
  auto successMessage() -> void final;
  auto updateMode() -> void final;
};

#endif // CADASTROPRODUTO_H
