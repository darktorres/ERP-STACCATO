#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui {
class LoginDialog;
}

class LoginDialog final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Login, Autorizacao };
  explicit LoginDialog(const Tipo tipo = Tipo::Login, QWidget *parent = nullptr);
  ~LoginDialog();

private:
  // attributes
  const Tipo tipo;
  Ui::LoginDialog *ui;
  // methods
  auto on_comboBoxLoja_currentTextChanged(const QString &loja) -> void;
  auto on_lineEditHostname_textChanged(const QString &) -> void;
  auto on_pushButtonConfig_clicked() -> void;
  auto on_pushButtonLogin_clicked() -> void;
  auto setComboBox() -> void;
};

#endif // LOGINDIALOG_H
