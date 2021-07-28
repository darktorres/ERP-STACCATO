#pragma once

#include <QDialog>

namespace Ui {
class ValidadeDialog;
}

class ValidadeDialog final : public QDialog {
  Q_OBJECT

public:
  explicit ValidadeDialog(QWidget *parent);
  ~ValidadeDialog();

  auto getValidade() -> int;

private:
  // attributes
  Ui::ValidadeDialog *ui;
  // methods
  auto on_checkBoxSemValidade_toggled(bool checked) -> void;
  auto on_dateEdit_dateChanged(const QDate date) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_spinBox_valueChanged(const int dias) -> void;
  auto setConnections() -> void;
};
