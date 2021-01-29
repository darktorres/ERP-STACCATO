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

  getSettings();

  if (UserSession::tipoUsuario == "VENDEDOR" or UserSession::tipoUsuario == "VENDEDOR ESPECIAL") { hideWidgets(); }

  setConnections();

  adjustSize();
}

UserConfig::~UserConfig() { delete ui; }

void UserConfig::getSettings() {
  ui->lineEditACBrServidor->setText(UserSession::getSetting("User/servidorACBr").toString());
  ui->lineEditACBrPorta->setText(UserSession::getSetting("User/portaACBr").toString());
  ui->itemBoxLoja->setId(UserSession::getSetting("User/lojaACBr"));
  ui->lineEditEmailContabilidade->setText(UserSession::getSetting("User/emailContabilidade").toString());
  ui->lineEditEmailLogistica->setText(UserSession::getSetting("User/emailLogistica").toString());
  ui->checkBoxMonitorarNFes->setChecked(UserSession::getSetting("User/monitorarNFe").toBool());

  ui->lineEditServidorSMTP->setText(UserSession::getSetting("User/servidorSMTP").toString());
  ui->lineEditPortaSMTP->setText(UserSession::getSetting("User/portaSMTP").toString());
  ui->lineEditEmail->setText(UserSession::getSetting("User/emailCompra").toString());
  ui->lineEditEmailSenha->setText(UserSession::getSetting("User/emailSenha").toString());
  ui->lineEditEmailCopia->setText(UserSession::getSetting("User/emailCopia").toString());

  ui->lineEditOrcamentosFolder->setText(UserSession::getSetting("User/OrcamentosFolder").toString());
  ui->lineEditVendasFolder->setText(UserSession::getSetting("User/VendasFolder").toString());
  ui->lineEditComprasFolder->setText(UserSession::getSetting("User/ComprasFolder").toString());
  ui->lineEditEntregasXmlFolder->setText(UserSession::getSetting("User/EntregasXmlFolder").toString());
  ui->lineEditEntregasPdfFolder->setText(UserSession::getSetting("User/EntregasPdfFolder").toString());
}

void UserConfig::hideWidgets() {
  ui->groupBoxAcbr->hide();
  ui->groupBoxEmail->hide();
  ui->labelCompras->hide();
  ui->labelEntregas->hide();
  ui->labelEntregas_2->hide();
  ui->lineEditComprasFolder->hide();
  ui->lineEditEntregasPdfFolder->hide();
  ui->lineEditEntregasXmlFolder->hide();
  ui->pushButtonComprasFolder->hide();
  ui->pushButtonEntregasPdfFolder->hide();
  ui->pushButtonEntregasXmlFolder->hide();
}

void UserConfig::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonAlterarDados, &QPushButton::clicked, this, &UserConfig::on_pushButtonAlterarDados_clicked, connectionType);
  connect(ui->pushButtonComprasFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonComprasFolder_clicked, connectionType);
  connect(ui->pushButtonEmailTeste, &QPushButton::clicked, this, &UserConfig::on_pushButtonEmailTeste_clicked, connectionType);
  connect(ui->pushButtonEntregasPdfFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonEntregasPdfFolder_clicked, connectionType);
  connect(ui->pushButtonEntregasXmlFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonEntregasXmlFolder_clicked, connectionType);
  connect(ui->pushButtonOrcamentosFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonOrcamentosFolder_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &UserConfig::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->pushButtonVendasFolder, &QPushButton::clicked, this, &UserConfig::on_pushButtonVendasFolder_clicked, connectionType);
}

void UserConfig::on_pushButtonOrcamentosFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel");

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

  // TODO: caso as pastas estejam vazias usar /arquivos como padrao
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
  usuario->viewRegisterById(UserSession::idUsuario);
  usuario->modificarUsuario();

  usuario->show();
}

void UserConfig::on_pushButtonVendasFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel");

  if (path.isEmpty()) { return; }

  ui->lineEditVendasFolder->setText(path);
}

void UserConfig::on_pushButtonComprasFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel");

  if (path.isEmpty()) { return; }

  ui->lineEditComprasFolder->setText(path);
}

void UserConfig::on_pushButtonEntregasXmlFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta XML");

  if (path.isEmpty()) { return; }

  ui->lineEditEntregasXmlFolder->setText(path);
}

void UserConfig::on_pushButtonEntregasPdfFolder_clicked() {
  const QString path = QFileDialog::getExistingDirectory(this, "Pasta PDF/Excel");

  if (path.isEmpty()) { return; }

  ui->lineEditEntregasPdfFolder->setText(path);
}

void UserConfig::on_pushButtonEmailTeste_clicked() {
  if (ui->lineEditServidorSMTP->text().isEmpty() or ui->lineEditPortaSMTP->text().isEmpty() or ui->lineEditEmail->text().isEmpty() or ui->lineEditEmailSenha->text().isEmpty()) {
    throw RuntimeError("Preencha os dados do email!", this);
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
