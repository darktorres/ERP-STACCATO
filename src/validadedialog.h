#ifndef VALIDADEDIALOG_H
#define VALIDADEDIALOG_H

#include <QDialog>

namespace Ui {
class ValidadeDialog;
}

class ValidadeDialog final : public QDialog {
  Q_OBJECT

public:
  explicit ValidadeDialog(QWidget *parent = nullptr);
  ~ValidadeDialog();
  auto getValidade() -> int;

private:
  // attributes
  Ui::ValidadeDialog *ui;
  // methods
  auto on_dateEdit_dateChanged(const QDate &date) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_spinBox_valueChanged(const int dias) -> void;
};

#endif // VALIDADEDIALOG_H
