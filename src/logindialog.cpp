#include "logindialog.h"
#include "ui_logindialog.h"

#include "application.h"
#include "usersession.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVersionNumber>

LoginDialog::LoginDialog(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::LoginDialog) {
  ui->setupUi(this);

  connect(ui->comboBoxLoja, &QComboBox::currentTextChanged, this, &LoginDialog::on_comboBoxLoja_currentTextChanged);
  connect(ui->lineEditHostname, &QLineEdit::textChanged, this, &LoginDialog::on_lineEditHostname_textChanged);
  connect(ui->pushButtonConfig, &QPushButton::clicked, this, &LoginDialog::on_pushButtonConfig_clicked);
  connect(ui->pushButtonLogin, &QPushButton::clicked, this, &LoginDialog::on_pushButtonLogin_clicked);

  setWindowTitle("ERP Login");
  setWindowModality(Qt::WindowModal);

  setComboBox();

  ui->lineEditUser->setFocus();

  if (not UserSession::getSetting("User/lastuser").toString().isEmpty()) {
    ui->lineEditUser->setText(UserSession::getSetting("User/lastuser").toString());
    ui->lineEditPass->setFocus();
  }

  if (not UserSession::getSetting("Login/hostname").toString().isEmpty()) { ui->lineEditHostname->setText(UserSession::getSetting("Login/hostname").toString()); }

  if (not UserSession::getSetting("Login/loja").toString().isEmpty()) { ui->comboBoxLoja->setCurrentText(UserSession::getSetting("Login/loja").toString()); }

  ui->checkBoxSalvarSenha->hide();
  ui->labelHostname->hide();
  ui->lineEditHostname->hide();
  ui->comboBoxLoja->hide();

  if (tipo == Tipo::Login) {
    const bool salvarSenha = UserSession::getSetting("User/savePasswd").toBool();

    ui->checkBoxSalvarSenha->setChecked(salvarSenha);

    if (salvarSenha) {
      const QString passwd = UserSession::getSetting("User/passwd").toString();
      ui->lineEditPass->setText(passwd);
      ui->pushButtonLogin->setFocus();
    }
  }

  if (tipo == Tipo::Autorizacao) {
    ui->pushButtonLogin->setText("Autorizar");
    ui->pushButtonConfig->hide();
    ui->lineEditUser->clear();
    ui->lineEditUser->setFocus();
    setWindowTitle("Autorização");
  }

  adjustSize();
}

void LoginDialog::setComboBox() {
  const auto keys = qApp->getMapLojas().keys();

  for (const auto &loja : keys) { ui->comboBoxLoja->addItem(loja); }
}

LoginDialog::~LoginDialog() { delete ui; }

void LoginDialog::on_pushButtonConfig_clicked() {
  ui->checkBoxSalvarSenha->setVisible(not ui->checkBoxSalvarSenha->isVisible());
  ui->labelHostname->setVisible(not ui->labelHostname->isVisible());
  ui->lineEditHostname->setVisible(not ui->lineEditHostname->isVisible());
  ui->comboBoxLoja->setVisible(not ui->comboBoxLoja->isVisible());
  adjustSize();
}

void LoginDialog::on_pushButtonLogin_clicked() {
  if (tipo == Tipo::Login) {
    UserSession::setSetting("User/savePasswd", ui->checkBoxSalvarSenha->isChecked());

    UserSession::setSetting("Login/hostname", ui->lineEditHostname->text());
    UserSession::setSetting("User/lastuser", ui->lineEditUser->text());
    UserSession::setSetting("User/passwd", ui->lineEditPass->text());

    if (not qApp->dbConnect(ui->lineEditHostname->text(), ui->lineEditUser->text().toLower(), ui->lineEditPass->text())) { return; }

    verificaVersao();
  }

  if (tipo == Tipo::Autorizacao) { UserSession::autorizacao(ui->lineEditUser->text(), ui->lineEditPass->text()); }

  accept();
}

void LoginDialog::verificaVersao() {
  QSqlQuery query;

  if (not query.exec("SELECT versaoAtual FROM versao_erp") or not query.first()) { throw RuntimeException("Erro verificando versão atual: " + query.lastError().text()); }

  QVersionNumber currentVersion = QVersionNumber::fromString(qApp->applicationVersion());
  QVersionNumber serverVersion = QVersionNumber::fromString(query.value("versaoAtual").toString());

  if (currentVersion < serverVersion) {
    throw RuntimeError("Versão do ERP não é a mais recente!\nSua versão: " + qApp->applicationVersion() + "\nVersão atual: " + query.value("versaoAtual").toString());
  }
}

void LoginDialog::on_comboBoxLoja_currentTextChanged(const QString &loja) {
  UserSession::setSetting("Login/loja", loja);
  ui->lineEditHostname->setText(qApp->getMapLojas().value(loja));
}

void LoginDialog::on_lineEditHostname_textChanged(const QString &hostname) {
  UserSession::setSetting("Login/hostname", hostname);
  qApp->updater();
}
