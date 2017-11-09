#include <QInputDialog>
#include <QMessageBox>

#include "logindialog.h"
#include "qsimpleupdater.h"
#include "ui_logindialog.h"
#include "usersession.h"

LoginDialog::LoginDialog(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::LoginDialog) {
  ui->setupUi(this);

  setWindowTitle("ERP Login");
  setWindowModality(Qt::WindowModal);

  readSettingsFile();
  setComboBox();

  ui->lineEditUser->setFocus();

  if (UserSession::settingsContains("User/lastuser")) {
    ui->lineEditUser->setText(UserSession::getSetting("User/lastuser").toString());
    ui->lineEditPass->setFocus();
  }

  ui->lineEditHostname->setText(UserSession::getSetting("Login/hostname").toString());

  ui->labelHostname->hide();
  ui->lineEditHostname->hide();
  ui->comboBoxLoja->hide();

  if (tipo == Tipo::Autorizacao) {
    ui->pushButtonConfig->hide();
    ui->lineEditUser->clear();
    ui->lineEditUser->setFocus();
    setWindowTitle("Autorização");
  }

  storeSelection();
  adjustSize();
  accept();
}

void LoginDialog::readSettingsFile() {
  QFile file("lojas.txt");

  if (not file.open(QFile::ReadOnly)) {
    QMessageBox::critical(this, "Erro!", "Erro lendo configurações: " + file.errorString());
    return;
  }

  const QStringList lines = QString(file.readAll()).split("\r\n", QString::SkipEmptyParts);

  for (int i = 0; i < lines.size(); i += 2) mapLojas.insert(lines.at(i), lines.at(i + 1));
}

void LoginDialog::setComboBox() {
  for (const auto &loja : mapLojas.keys()) ui->comboBoxLoja->addItem(loja);
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

  if (not UserSession::login(ui->lineEditUser->text(), ui->lineEditPass->text(), tipo == Tipo::Autorizacao ? UserSession::Tipo::Autorizacao : UserSession::Tipo::Padrao)) {
    QMessageBox::critical(this, "Erro!", "Login inválido!");
    ui->lineEditPass->setFocus();
    return;
  }

  accept();

  if (tipo == Tipo::Login) UserSession::setSetting("User/lastuser", ui->lineEditUser->text());
}

void LoginDialog::updater() {
  auto *updater = new QSimpleUpdater();
  updater->setApplicationVersion(qApp->applicationVersion());
  updater->setReferenceUrl("http://" + UserSession::getSetting("Login/hostname").toString() + "/versao.txt");
  updater->setDownloadUrl("http://" + UserSession::getSetting("Login/hostname").toString() + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
}

void LoginDialog::storeSelection() {
  if (UserSession::getSetting("Login/hostname").toString().isEmpty()) {
    const QStringList items = mapLojas.keys();

    const QString loja = QInputDialog::getItem(nullptr, "Escolha a loja", "Qual a sua loja?", items, 0, false);

    UserSession::setSetting("Login/hostname", mapLojas.value(loja));
    ui->lineEditHostname->setText(mapLojas.value(loja));
  }
}

void LoginDialog::on_comboBoxLoja_currentTextChanged(const QString &loja) { ui->lineEditHostname->setText(mapLojas.value(loja)); }

void LoginDialog::on_lineEditHostname_textChanged(const QString &) {
  UserSession::setSetting("Login/hostname", ui->lineEditHostname->text());
  updater();
}
