#include "userconfig.h"
#include "ui_userconfig.h"

#include "application.h"
#include "cadastrousuario.h"
#include "sendmail.h"
#include "usersession.h"

#include <QDebug>
#include <QFileDialog>

UserConfig::UserConfig(QWidget *parent) : QDialog(parent), ui(new Ui::UserConfig) {
  ui->setupUi(this);

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));

  if (const auto key = UserSession::getSetting("User/servidorACBr")) { ui->lineEditACBrServidor->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/portaACBr")) { ui->lineEditACBrPorta->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/lojaACBr")) { ui->itemBoxLoja->setId(key->toInt()); }
  if (const auto key = UserSession::getSetting("User/emailContabilidade")) { ui->lineEditEmailContabilidade->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/emailLogistica")) { ui->lineEditEmailLogistica->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/monitorarNFe")) { ui->checkBoxMonitorarNFes->setChecked(key->toBool()); }

  if (const auto key = UserSession::getSetting("User/servidorSMTP")) { ui->lineEditServidorSMTP->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/portaSMTP")) { ui->lineEditPortaSMTP->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/emailCompra")) { ui->lineEditEmail->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/emailSenha")) { ui->lineEditEmailSenha->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/emailCopia")) { ui->lineEditEmailCopia->setText(key->toString()); }

  if (const auto key = UserSession::getSetting("User/OrcamentosFolder")) { ui->lineEditOrcamentosFolder->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/VendasFolder")) { ui->lineEditVendasFolder->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/ComprasFolder")) { ui->lineEditComprasFolder->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/EntregasXmlFolder")) { ui->lineEditEntregasXmlFolder->setText(key->toString()); }
  if (const auto key = UserSession::getSetting("User/EntregasPdfFolder")) { ui->lineEditEntregasPdfFolder->setText(key->toString()); }

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

  connect(ui->pushButtonAlterarDados, &QPushButton::clicked, this, &UserConfig::on_pushButtonAlterarDados_clicked);
  connect(ui->pushButtonComprasFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonComprasFolder_clicked);
  connect(ui->pushButtonEmailTeste, &QPushButton::clicked, this, &UserConfig::on_pushButtonEmailTeste_clicked);
  connect(ui->pushButtonEntregasPdfFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonEntregasPdfFolder_clicked);
  connect(ui->pushButtonEntregasXmlFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonEntregasXmlFolder_clicked);
  connect(ui->pushButtonOrcamentosFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonOrcamentosFolder_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &UserConfig::on_pushButtonSalvar_clicked);
  connect(ui->pushButtonVendasFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonVendasFolder_clicked);

  adjustSize();
}

UserConfig::~UserConfig() { delete ui; }

void UserConfig::on_pushButtonOrcamentosFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) { return; }

  ui->lineEditOrcamentosFolder->setText(path);
}

void UserConfig::on_pushButtonSalvar_clicked() {
  UserSession::setSetting("User/servidorACBr", ui->lineEditACBrServidor->text());
  UserSession::setSetting("User/portaACBr", ui->lineEditACBrPorta->text());
  UserSession::setSetting("User/lojaACBr", ui->itemBoxLoja->getId());
  UserSession::setSetting("User/emailContabilidade", ui->lineEditEmailContabilidade->text());
  UserSession::setSetting("User/emailLogistica", ui->lineEditEmailLogistica->text());
  UserSession::setSetting("User/monitorarNFe", ui->checkBoxMonitorarNFes->isChecked());

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

  usuario->show();
}

void UserConfig::on_pushButtonVendasFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) { return; }

  ui->lineEditVendasFolder->setText(path);
}

void UserConfig::on_pushButtonComprasFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) { return; }

  ui->lineEditComprasFolder->setText(path);
}

void UserConfig::on_pushButtonEntregasXmlFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta XML", QDir::currentPath());

  if (path.isEmpty()) { return; }

  ui->lineEditEntregasXmlFolder->setText(path);
}

void UserConfig::on_pushButtonEntregasPdfFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel", QDir::currentPath());

  if (path.isEmpty()) { return; }

  ui->lineEditEntregasPdfFolder->setText(path);
}

void UserConfig::on_pushButtonEmailTeste_clicked() {
  if (ui->lineEditServidorSMTP->text().isEmpty() or ui->lineEditPortaSMTP->text().isEmpty() or ui->lineEditEmail->text().isEmpty() or ui->lineEditEmailSenha->text().isEmpty()) {
    return qApp->enqueueError("Preencha os dados do email!", this);
  }

  UserSession::setSetting("User/servidorSMTP", ui->lineEditServidorSMTP->text());
  UserSession::setSetting("User/portaSMTP", ui->lineEditPortaSMTP->text());
  UserSession::setSetting("User/emailCompra", ui->lineEditEmail->text());
  UserSession::setSetting("User/emailSenha", ui->lineEditEmailSenha->text());
  UserSession::setSetting("User/emailCopia", ui->lineEditEmailCopia->text());

  auto *mail = new SendMail(SendMail::Tipo::Teste, this);
  mail->setAttribute(Qt::WA_DeleteOnClose);

  mail->show();
}
