#pragma once

#include <QDialog>

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
  Tipo const tipo;
  Ui::LoginDialog *ui;
  // methods
  auto on_comboBoxLoja_currentTextChanged(const QString &loja) -> void;
  auto on_lineEditHostname_textChanged(const QString &hostname) -> void;
  auto on_pushButtonConfig_clicked() -> void;
  auto on_pushButtonLogin_clicked() -> void;
  auto setComboBox() -> void;
  auto setConnections() -> void;
  auto verificaManutencao() -> void;
  auto verificaVersao() -> void;
};
