#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class InputDialog;
}

class InputDialog final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Carrinho, Faturamento, AgendarColeta, Coleta, AgendarRecebimento, AgendarEntrega, ReagendarPedido };

  explicit InputDialog(const Tipo &tipo, QWidget *parent = nullptr);
  ~InputDialog();
  QDate getDate() const;
  QDate getNextDate() const;
  QString getObservacao() const;

private slots:
  void on_dateEditEvento_dateChanged(const QDate &date);
  void on_pushButtonSalvar_clicked();

private:
  // attributes
  const Tipo tipo;
  Ui::InputDialog *ui;
  // methods
};

#endif // INPUTDIALOG_H
