#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSimpleUpdater>
#include <QSqlError>
#include <QStyleFactory>

#include "logindialog.h"
#include "ui_logindialog.h"
#include "usersession.h"

LoginDialog::LoginDialog(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::LoginDialog) {
  ui->setupUi(this);

  if (UserSession::settings("User/tema").toString() == "escuro") darkTheme();

  setWindowTitle("ERP Login");
  setWindowModality(Qt::WindowModal);

  readSettingsFile();

  ui->lineEditUser->setFocus();

  if (UserSession::settingsContains("User/lastuser")) {
    ui->lineEditUser->setText(UserSession::settings("User/lastuser").toString());
    ui->lineEditPass->setFocus();
  }

  ui->lineEditHostname->setText(UserSession::settings("Login/hostname").toString());

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

  const QStringList lines = QString(file.readAll()).split("\r\n");

  for (int i = 0; i < lines.size(); i += 2) hash.insert(lines.at(i), lines.at(i + 1));
}

LoginDialog::~LoginDialog() { delete ui; }

void LoginDialog::darkTheme() {
  qApp->setStyle(QStyleFactory::create("Fusion"));

  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
  darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 120));

  darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  qApp->setPalette(darkPalette);

  qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
}

void LoginDialog::on_pushButtonConfig_clicked() {
  ui->labelHostname->setVisible(not ui->labelHostname->isVisible());
  ui->lineEditHostname->setVisible(not ui->lineEditHostname->isVisible());
  ui->comboBoxLoja->setVisible(not ui->comboBoxLoja->isVisible());
  adjustSize();
}

bool LoginDialog::dbConnect() {
  if (not QSqlDatabase::drivers().contains("QMYSQL")) {
    QMessageBox::critical(this, "Não foi possível carregar o banco de dados.", "Este aplicativo requer o driver QMYSQL.");
    exit(1);
  }

  QSqlDatabase db = QSqlDatabase::contains() ? QSqlDatabase::database() : QSqlDatabase::addDatabase("QMYSQL");

  db.setHostName(UserSession::settings("Login/hostname").toString());
  db.setUserName(ui->lineEditUser->text().toLower());
  db.setPassword("1234");
  db.setDatabaseName("mysql");

  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_RECONNECT=1");
  //  db.setConnectOptions("CLIENT_COMPRESS=1;MYSQL_OPT_RECONNECT=1;MYSQL_OPT_CONNECT_TIMEOUT=60;MYSQL_OPT_READ_TIMEOUT=60;"
  //                       "MYSQL_OPT_WRITE_TIMEOUT=60");

  if (not db.open()) {
    QString message;

    switch (db.lastError().number()) {
    case 1045:
      message = "Verifique se o usuário e senha do banco de dados estão corretos.";
      break;

    case 2002:
      message = "Verifique se o servidor está ligado, e acessível pela rede.";
      break;

    case 2003:
      message = "Verifique se o servidor está ligado, e acessível pela rede.";
      break;

    case 2005:
      message = "Verifique se o IP do servidor foi escrito corretamente.";
      break;

    default:
      message = "Erro conectando no banco de dados: " + db.lastError().text();
      break;
    }

    QMessageBox::critical(this, "Erro: Banco de dados inacessível!", message);

    return false;
  }

  QSqlQuery query = db.exec("SHOW SCHEMAS");
  bool hasMydb = false;

  while (query.next()) {
    if (query.value(0).toString() == "mydb") hasMydb = true;
  }

  if (not hasMydb) {
    QMessageBox::critical(this, "Erro!", "Não encontrou as tabelas do bando de dados, verifique se o servidor está funcionando corretamente.");
    return false;
  }

  db.close();

  db.setDatabaseName("mydb");

  if (not db.open()) {
    QMessageBox::critical(this, "Erro", "Erro conectando no banco de dados: " + db.lastError().text());
    return false;
  }

  if (not query.exec("SELECT lastInvalidated FROM maintenance") or not query.first()) {
    QMessageBox::critical(this, "Erro", "Erro verificando lastInvalidated: " + query.lastError().text());
    return false;
  }

  if (query.value("lastInvalidated").toDate() < QDate::currentDate()) {
    if (not query.exec("CALL invalidate_expired()")) {
      QMessageBox::critical(this, "Erro!", "Erro executando InvalidarExpirados: " + query.lastError().text());
      return false;
    }

    query.prepare("UPDATE maintenance SET lastInvalidated = :lastInvalidated WHERE id = 1");
    query.bindValue(":lastInvalidated", QDate::currentDate().toString("yyyy-MM-dd"));

    if (not query.exec()) {
      QMessageBox::critical(this, "Erro", "Erro atualizando lastInvalidated: " + query.lastError().text());
      return false;
    }
  }

  if (not query.exec("CALL update_orcamento_status()")) {
    QMessageBox::critical(this, "Erro!", "Erro executando update_orcamento_status: " + query.lastError().text());
    return false;
  }

  return true;
}

void LoginDialog::on_pushButtonLogin_clicked() {
  if (tipo == Tipo::Login and not dbConnect()) return;

  if (not UserSession::login(ui->lineEditUser->text(), ui->lineEditPass->text(), tipo == Tipo::Autorizacao ? UserSession::Tipo::Autorizacao : UserSession::Tipo::Padrao)) {
    QMessageBox::critical(this, "Erro!", "Login inválido!");
    ui->lineEditPass->setFocus();
    return;
  }

  accept();

  if (tipo == Tipo::Login) UserSession::setSettings("User/lastuser", ui->lineEditUser->text());
}

void LoginDialog::updater() {
  auto *updater = new QSimpleUpdater();
  updater->setApplicationVersion(qApp->applicationVersion());
  updater->setReferenceUrl("http://" + UserSession::settings("Login/hostname").toString() + "/versao.txt");
  updater->setDownloadUrl("http://" + UserSession::settings("Login/hostname").toString() + "/Instalador.exe");
  updater->setSilent(true);
  updater->setShowNewestVersionMessage(true);
  updater->checkForUpdates();
}

void LoginDialog::storeSelection() {
  if (UserSession::settings("Login/hostname").toString().isEmpty()) {
    const QStringList items = {"Alphaville", "Gabriel", "Granja"};

    const QString loja = QInputDialog::getItem(nullptr, "Escolha a loja", "Qual a sua loja?", items, 0, false);

    UserSession::setSettings("Login/hostname", hash.value(loja));
  }
}

void LoginDialog::on_comboBoxLoja_currentTextChanged(const QString &loja) { ui->lineEditHostname->setText(hash.value(loja)); }

void LoginDialog::on_lineEditHostname_textChanged(const QString &) {
  UserSession::setSettings("Login/hostname", ui->lineEditHostname->text());
  updater();
}
