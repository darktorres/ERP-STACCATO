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
  explicit CadastroProduto(QWidget *parent = 0);
  ~CadastroProduto();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

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
  bool cadastrar() final;
  bool save() final;
  bool savingProcedures() final;
  bool verifyFields() final;
  bool viewRegister() final;
  void clearFields() final;
  void registerMode() final;
  void setupMapper() final;
  void successMessage() final;
  void updateMode() final;
  void calcularMarkup();
};

#endif // CADASTROPRODUTO_H
