#include <QDebug>
#include <QFileDialog>

#include "cadastrousuario.h"
#include "ui_userconfig.h"
#include "userconfig.h"
#include "usersession.h"

UserConfig::UserConfig(QWidget *parent) : QDialog(parent), ui(new Ui::UserConfig) {
  ui->setupUi(this);

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
  ui->itemBoxLoja->setValue(UserSession::setSetting("User/lojaACBr"));
  // REFAC: remove these two
  ui->lineEditEmailContabilidade->setText(UserSession::setSetting("User/emailContabilidade").toString());
  ui->lineEditEmailLogistica->setText(UserSession::setSetting("User/emailLogistica").toString());

  ui->lineEditServidorSMTP->setText(UserSession::setSetting("User/servidorSMTP").toString());
  ui->lineEditPortaSMTP->setText(UserSession::setSetting("User/portaSMTP").toString());
  // REFAC: rename this to email
  ui->lineEditEmail->setText(UserSession::setSetting("User/emailCompra").toString());
  ui->lineEditEmailSenha->setText(UserSession::setSetting("User/emailSenha").toString());
  ui->lineEditEmailCopia->setText(UserSession::setSetting("User/emailCopia").toString());

  ui->lineEditOrcamentosFolder->setText(UserSession::setSetting("User/OrcamentosFolder").toString());
  ui->lineEditVendasFolder->setText(UserSession::setSetting("User/VendasFolder").toString());
  ui->lineEditComprasFolder->setText(UserSession::setSetting("User/ComprasFolder").toString());
  ui->lineEditEntregasXmlFolder->setText(UserSession::setSetting("User/EntregasXmlFolder").toString());
  ui->lineEditEntregasPdfFolder->setText(UserSession::setSetting("User/EntregasPdfFolder").toString());

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
  UserSession::getSetting("User/lojaACBr", ui->itemBoxLoja->getValue());
  UserSession::getSetting("User/emailContabilidade", ui->lineEditEmailContabilidade->text());
  UserSession::getSetting("User/emailLogistica", ui->lineEditEmailLogistica->text());

  UserSession::getSetting("User/servidorSMTP", ui->lineEditServidorSMTP->text());
  UserSession::getSetting("User/portaSMTP", ui->lineEditPortaSMTP->text());
  UserSession::getSetting("User/emailCompra", ui->lineEditEmail->text());
  UserSession::getSetting("User/emailSenha", ui->lineEditEmailSenha->text());
  UserSession::getSetting("User/emailCopia", ui->lineEditEmailCopia->text());

  UserSession::getSetting("User/OrcamentosFolder", ui->lineEditOrcamentosFolder->text());
  UserSession::getSetting("User/VendasFolder", ui->lineEditVendasFolder->text());
  UserSession::getSetting("User/ComprasFolder", ui->lineEditComprasFolder->text());
  UserSession::getSetting("User/EntregasXmlFolder", ui->lineEditEntregasXmlFolder->text());
  UserSession::getSetting("User/EntregasPdfFolder", ui->lineEditEntregasPdfFolder->text());

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
