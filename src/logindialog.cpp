#include "logindialog.h"
#include "ui_logindialog.h"

#include "application.h"
#include "log.h"
#include "user.h"

#include <QHostInfo>
#include <QMessageBox>
#include <QNetworkInterface>
#include <QSqlError>
#include <QVersionNumber>

LoginDialog::LoginDialog(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::LoginDialog) {
  ui->setupUi(this);

  setWindowTitle("ERP Login");
  setWindowModality(Qt::WindowModal);

  setComboBox();

  ui->lineEditUser->setFocus();

  if (not User::getSetting("User/lastuser").toString().isEmpty()) {
    ui->lineEditUser->setText(User::getSetting("User/lastuser").toString());
    ui->lineEditPass->setFocus();
  }

  if (not User::getSetting("Login/hostname").toString().isEmpty()) { ui->lineEditHostname->setText(User::getSetting("Login/hostname").toString()); }

  if (not User::getSetting("Login/loja").toString().isEmpty()) { ui->comboBoxLoja->setCurrentText(User::getSetting("Login/loja").toString()); }

  ui->checkBoxSalvarSenha->hide();
  ui->labelHostname->hide();
  ui->lineEditHostname->hide();
  ui->comboBoxLoja->hide();

  if (tipo == Tipo::Login) {
    const bool salvarSenha = User::getSetting("User/savePasswd").toBool();

    ui->checkBoxSalvarSenha->setChecked(salvarSenha);

    if (salvarSenha) {
      const QString passwd = User::getSetting("User/passwd").toString();
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

  qApp->updater();

  setConnections();

  adjustSize();
}

void LoginDialog::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->comboBoxLoja, &QComboBox::currentTextChanged, this, &LoginDialog::on_comboBoxLoja_currentTextChanged, connectionType);
  connect(ui->lineEditHostname, &QLineEdit::textChanged, this, &LoginDialog::on_lineEditHostname_textChanged, connectionType);
  connect(ui->pushButtonConfig, &QPushButton::clicked, this, &LoginDialog::on_pushButtonConfig_clicked, connectionType);
  connect(ui->pushButtonLogin, &QPushButton::clicked, this, &LoginDialog::on_pushButtonLogin_clicked, connectionType);
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
    User::setSetting("User/savePasswd", ui->checkBoxSalvarSenha->isChecked());

    User::setSetting("Login/hostname", ui->lineEditHostname->text());
    User::setSetting("User/lastuser", ui->lineEditUser->text());
    User::setSetting("User/passwd", ui->lineEditPass->text());

    qApp->dbConnect(ui->lineEditHostname->text(), ui->lineEditUser->text().toLower(), ui->lineEditPass->text());

    //-----------------------------------------------------

    auto addresses = QNetworkInterface::allAddresses();

    QMutableListIterator<QHostAddress> iterator(addresses);

    while (iterator.hasNext()) {
      if (auto address = iterator.next(); address.protocol() != QAbstractSocket::IPv4Protocol or address == QHostAddress(QHostAddress::LocalHost)) { iterator.remove(); }
    }

    //-----------------------------------------------------

    Log::createLog("Login", "Usuário: " + User::nome + ", Host: " + QHostInfo::localHostName() + ", IP: " + addresses.first().toString());

    verificaVersao(); // TODO: usar o webserver para indicar se está em manutencao
    verificaManutencao();
  }

  if (tipo == Tipo::Autorizacao) { User::autorizacao(ui->lineEditUser->text(), ui->lineEditPass->text()); }

  accept();
}

void LoginDialog::verificaManutencao() {
  SqlQuery query;

  if (not query.exec("SELECT emManutencao FROM maintenance")) { throw RuntimeException("Erro verificando se em manutenção: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Erro verificando se em manutenção!"); }

  if (query.value("emManutencao").toBool()) { throw RuntimeException("Sistema em manutenção!"); }
}

void LoginDialog::verificaVersao() {
  SqlQuery query;

  if (not query.exec("SELECT versaoAtual FROM versao_erp")) { throw RuntimeException("Erro verificando versão atual: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Versão atual não encontrada!"); }

  QVersionNumber currentVersion = QVersionNumber::fromString(qApp->applicationVersion());
  QVersionNumber serverVersion = QVersionNumber::fromString(query.value("versaoAtual").toString());

  if (currentVersion < serverVersion) {
    throw RuntimeError("<pre>Vers&atilde;o do ERP n&atilde;o &eacute; a mais recente!<br />Sua vers&atilde;o: " + qApp->applicationVersion() +
                       " <br />Vers&atilde;o atual: " + query.value("versaoAtual").toString() +
                       "</pre><p><a "
                       "href=\"http://" +
                       qApp->getWebDavIp() + "/Instalador.exe\">Clique aqui para baixar a última versão</a></p>");
  }
}

void LoginDialog::on_comboBoxLoja_currentTextChanged(const QString &loja) {
  User::setSetting("Login/loja", loja);
  ui->lineEditHostname->setText(qApp->getMapLojas().value(loja));
}

void LoginDialog::on_lineEditHostname_textChanged(const QString &hostname) {
  User::setSetting("Login/hostname", hostname);
  qApp->updater();
}
