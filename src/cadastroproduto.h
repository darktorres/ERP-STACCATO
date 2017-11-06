#ifndef CADASTROPRODUTO_H
#define CADASTROPRODUTO_H

#include "registerdialog.h"
#include "searchdialog.h"

namespace Ui {
class CadastroProduto;
}

class CadastroProduto : public RegisterDialog {
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
  virtual bool cadastrar() override;
  virtual bool save() override;
  virtual bool savingProcedures() override;
  virtual bool verifyFields() override;
  virtual bool viewRegister() override;
  virtual void clearFields() override;
  virtual void registerMode() override;
  virtual void setupMapper() override;
  virtual void successMessage() override;
  virtual void updateMode() override;
  void calcularMarkup();
};

#endif // CADASTROPRODUTO_H
