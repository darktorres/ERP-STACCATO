#include <QDebug>
#include <QFileDialog>

#include "cadastrousuario.h"
#include "ui_userconfig.h"
#include "userconfig.h"
#include "usersession.h"

UserConfig::UserConfig(QWidget *parent) : QDialog(parent), ui(new Ui::UserConfig) {
  ui->setupUi(this);

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
  ui->itemBoxLoja->setValue(UserSession::getSetting("User/lojaACBr"));
  // REFAC: remove these two
  ui->lineEditEmailContabilidade->setText(UserSession::getSetting("User/emailContabilidade").toString());
  ui->lineEditEmailLogistica->setText(UserSession::getSetting("User/emailLogistica").toString());

  ui->lineEditServidorSMTP->setText(UserSession::getSetting("User/servidorSMTP").toString());
  ui->lineEditPortaSMTP->setText(UserSession::getSetting("User/portaSMTP").toString());
  // REFAC: rename this to email
  ui->lineEditEmail->setText(UserSession::getSetting("User/emailCompra").toString());
  ui->lineEditEmailSenha->setText(UserSession::getSetting("User/emailSenha").toString());
  ui->lineEditEmailCopia->setText(UserSession::getSetting("User/emailCopia").toString());

  ui->lineEditOrcamentosFolder->setText(UserSession::getSetting("User/OrcamentosFolder").toString());
  ui->lineEditVendasFolder->setText(UserSession::getSetting("User/VendasFolder").toString());
  ui->lineEditComprasFolder->setText(UserSession::getSetting("User/ComprasFolder").toString());
  ui->lineEditEntregasXmlFolder->setText(UserSession::getSetting("User/EntregasXmlFolder").toString());
  ui->lineEditEntregasPdfFolder->setText(UserSession::getSetting("User/EntregasPdfFolder").toString());

  if (UserSession::tipoUsuario() == "VENDEDOR" or UserSession::tipoUsuario() == "VENDEDOR ESPECIAL") {
    ui->groupBoxAcbr->hide();
    ui->groupBoxEmail->hide();
    ui->labelCompras->hide();
    ui->lineEditComprasFolder->hide();
    ui->pushButtonComprasFolder->hide();
    ui->labelEntregas->hide();
    ui->lineEditEntregasPdfFolder->hide();
    ui->pushButtonEntregasPdfFolder->hide();
    ui->labelEntregas_2->hide();
    ui->lineEditEntregasXmlFolder->hide();
    ui->pushButtonEntregasXmlFolder->hide();
  }

  adjustSize();
}

UserConfig::~UserConfig() { delete ui; }

void UserConfig::on_pushButtonOrcamentosFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) return;

  ui->lineEditOrcamentosFolder->setText(path);
}

void UserConfig::on_pushButtonSalvar_clicked() {
  UserSession::setSetting("User/lojaACBr", ui->itemBoxLoja->getValue());
  UserSession::setSetting("User/emailContabilidade", ui->lineEditEmailContabilidade->text());
  UserSession::setSetting("User/emailLogistica", ui->lineEditEmailLogistica->text());

  UserSession::setSetting("User/servidorSMTP", ui->lineEditServidorSMTP->text());
  UserSession::setSetting("User/portaSMTP", ui->lineEditPortaSMTP->text());
  UserSession::setSetting("User/emailCompra", ui->lineEditEmail->text());
  UserSession::setSetting("User/emailSenha", ui->lineEditEmailSenha->text());
  UserSession::setSetting("User/emailCopia", ui->lineEditEmailCopia->text());

  UserSession::setSetting("User/OrcamentosFolder", ui->lineEditOrcamentosFolder->text());
  UserSession::setSetting("User/VendasFolder", ui->lineEditVendasFolder->text());
  UserSession::setSetting("User/ComprasFolder", ui->lineEditComprasFolder->text());
  UserSession::setSetting("User/EntregasXmlFolder", ui->lineEditEntregasXmlFolder->text());
  UserSession::setSetting("User/EntregasPdfFolder", ui->lineEditEntregasPdfFolder->text());

  QDialog::accept();

  close();
}

void UserConfig::on_pushButtonAlterarDados_clicked() {
  auto *usuario = new CadastroUsuario(this);
  usuario->viewRegisterById(UserSession::idUsuario());
  usuario->modificarUsuario();

  connect(usuario, &CadastroUsuario::errorSignal, this, &UserConfig::errorSignal);
  connect(usuario, &CadastroUsuario::transactionStarted, this, &UserConfig::transactionStarted);
  connect(usuario, &CadastroUsuario::transactionEnded, this, &UserConfig::transactionEnded);

  usuario->show();
}

void UserConfig::on_pushButtonVendasFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) return;

  ui->lineEditVendasFolder->setText(path);
}

void UserConfig::on_pushButtonComprasFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) return;

  ui->lineEditComprasFolder->setText(path);
}

void UserConfig::on_pushButtonEntregasXmlFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta XML", QDir::currentPath());

  if (path.isEmpty()) return;

  ui->lineEditEntregasXmlFolder->setText(path);
}

void UserConfig::on_pushButtonEntregasPdfFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) return;

  ui->lineEditEntregasPdfFolder->setText(path);
}
