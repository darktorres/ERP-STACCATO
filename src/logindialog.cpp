#include <QMessageBox>

#include "application.h"
#include "logindialog.h"
#include "ui_logindialog.h"
#include "usersession.h"

LoginDialog::LoginDialog(const Tipo tipo, QWidget *parent) : Dialog(parent), tipo(tipo), ui(new Ui::LoginDialog) {
  ui->setupUi(this);

  connect(ui->comboBoxLoja, &QComboBox::currentTextChanged, this, &LoginDialog::on_comboBoxLoja_currentTextChanged);
  connect(ui->lineEditHostname, &QLineEdit::textChanged, this, &LoginDialog::on_lineEditHostname_textChanged);
  connect(ui->pushButtonConfig, &QPushButton::clicked, this, &LoginDialog::on_pushButtonConfig_clicked);
  connect(ui->pushButtonLogin, &QPushButton::clicked, this, &LoginDialog::on_pushButtonLogin_clicked);

  setWindowTitle("ERP Login");
  setWindowModality(Qt::WindowModal);

  setComboBox();

  ui->lineEditUser->setFocus();

  if (const auto key = UserSession::getSetting("User/lastuser"); key) {
    ui->lineEditUser->setText(key.value().toString());
    ui->lineEditPass->setFocus();
  }

  if (const auto key = UserSession::getSetting("Login/hostname"); key) { ui->lineEditHostname->setText(key.value().toString()); }

  ui->labelHostname->hide();
  ui->lineEditHostname->hide();
  ui->comboBoxLoja->hide();

  if (tipo == Tipo::Autorizacao) {
    ui->pushButtonConfig->hide();
    ui->lineEditUser->clear();
    ui->lineEditUser->setFocus();
    setWindowTitle("Autorização");
  }

  adjustSize();
  accept();
}

void LoginDialog::setComboBox() {
  Q_FOREACH (const auto &loja, qApp->getMapLojas().keys()) { ui->comboBoxLoja->addItem(loja); }
}

LoginDialog::~LoginDialog() { delete ui; }

void LoginDialog::on_pushButtonConfig_clicked() {
  ui->labelHostname->setVisible(not ui->labelHostname->isVisible());
  ui->lineEditHostname->setVisible(not ui->lineEditHostname->isVisible());
  ui->comboBoxLoja->setVisible(not ui->comboBoxLoja->isVisible());
  adjustSize();
}

void LoginDialog::on_pushButtonLogin_clicked() {
  UserSession::setSetting("Login/hostname", ui->lineEditHostname->text());
  UserSession::setSetting("User/lastuser", ui->lineEditUser->text());

  if (not qApp->dbConnect()) { return; }

  if (not UserSession::login(ui->lineEditUser->text(), ui->lineEditPass->text(), tipo == Tipo::Autorizacao ? UserSession::Tipo::Autorizacao : UserSession::Tipo::Padrao)) {
    emit errorSignal("Login inválido!");
    ui->lineEditPass->setFocus();
    return;
  }

  accept();

  if (tipo == Tipo::Login) { UserSession::setSetting("User/lastuser", ui->lineEditUser->text()); }
}

void LoginDialog::on_comboBoxLoja_currentTextChanged(const QString &loja) { ui->lineEditHostname->setText(qApp->getMapLojas().value(loja)); }

void LoginDialog::on_lineEditHostname_textChanged(const QString &) {
  UserSession::setSetting("Login/hostname", ui->lineEditHostname->text());
  qApp->updater();
}
