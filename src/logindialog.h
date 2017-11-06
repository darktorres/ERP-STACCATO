#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Login, Autorizacao };
  explicit LoginDialog(const Tipo tipo = Tipo::Login, QWidget *parent = 0);
  ~LoginDialog();

private slots:
  void on_comboBoxLoja_currentTextChanged(const QString &loja);
  void on_lineEditHostname_textChanged(const QString &);
  void on_pushButtonConfig_clicked();
  void on_pushButtonLogin_clicked();

private:
  // attributes
  const Tipo tipo;
  QHash<QString, QString> hash;
  Ui::LoginDialog *ui;
  // methods
  void storeSelection();
  void updater();
  void readSettingsFile();
};

#endif // LOGINDIALOG_H
