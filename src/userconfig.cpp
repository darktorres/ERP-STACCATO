#include "userconfig.h"
#include "ui_userconfig.h"

#include "application.h"
#include "cadastrousuario.h"
#include "sendmail.h"
#include "user.h"

#include <QDebug>
#include <QFileDialog>

UserConfig::UserConfig(QWidget *parent) : QDialog(parent), ui(new Ui::UserConfig) {
  ui->setupUi(this);

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));

  getSettings();

  if (User::isVendedorOrEspecial()) { hideWidgets(); }

  setConnections();

  adjustSize();
}

UserConfig::~UserConfig() { delete ui; }

void UserConfig::getSettings() {
  ui->lineEditACBrServidor->setText(User::getSetting("User/servidorACBr").toString());
  ui->lineEditACBrPorta->setText(User::getSetting("User/portaACBr").toString());
  ui->itemBoxLoja->setId(User::getSetting("User/lojaACBr"));
  ui->lineEditEmailContabilidade->setText(User::getSetting("User/emailContabilidade").toString());
  ui->lineEditEmailLogistica->setText(User::getSetting("User/emailLogistica").toString());
  ui->checkBoxMonitorarNFes->setChecked(User::getSetting("User/monitorarNFe").toBool());

  ui->lineEditServidorSMTP->setText(User::getSetting("User/servidorSMTP").toString());
  ui->lineEditPortaSMTP->setText(User::getSetting("User/portaSMTP").toString());
  ui->lineEditEmail->setText(User::getSetting("User/emailCompra").toString());
  ui->lineEditEmailSenha->setText(User::getSetting("User/emailSenha").toString());
  ui->lineEditEmailCopia->setText(User::getSetting("User/emailCopia").toString());

  ui->lineEditOrcamentosFolder->setText(User::getSetting("User/OrcamentosFolder").toString());
  ui->lineEditVendasFolder->setText(User::getSetting("User/VendasFolder").toString());
  ui->lineEditComprasFolder->setText(User::getSetting("User/ComprasFolder").toString());
  ui->lineEditEntregasXmlFolder->setText(User::getSetting("User/EntregasXmlFolder").toString());
  ui->lineEditEntregasPdfFolder->setText(User::getSetting("User/EntregasPdfFolder").toString());
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
  User::setSetting("User/servidorACBr", ui->lineEditACBrServidor->text());
  User::setSetting("User/portaACBr", ui->lineEditACBrPorta->text());
  User::setSetting("User/lojaACBr", ui->itemBoxLoja->getId());
  User::setSetting("User/emailContabilidade", ui->lineEditEmailContabilidade->text());
  User::setSetting("User/emailLogistica", ui->lineEditEmailLogistica->text());
  User::setSetting("User/monitorarNFe", ui->checkBoxMonitorarNFes->isChecked());

  User::setSetting("User/servidorSMTP", ui->lineEditServidorSMTP->text());
  User::setSetting("User/portaSMTP", ui->lineEditPortaSMTP->text());
  User::setSetting("User/emailCompra", ui->lineEditEmail->text());
  User::setSetting("User/emailSenha", ui->lineEditEmailSenha->text());
  User::setSetting("User/emailCopia", ui->lineEditEmailCopia->text());

  // TODO: caso as pastas estejam vazias usar /arquivos como padrao
  User::setSetting("User/OrcamentosFolder", ui->lineEditOrcamentosFolder->text());
  User::setSetting("User/VendasFolder", ui->lineEditVendasFolder->text());
  User::setSetting("User/ComprasFolder", ui->lineEditComprasFolder->text());
  User::setSetting("User/EntregasXmlFolder", ui->lineEditEntregasXmlFolder->text());
  User::setSetting("User/EntregasPdfFolder", ui->lineEditEntregasPdfFolder->text());

  QDialog::accept();

  close();
}

void UserConfig::on_pushButtonAlterarDados_clicked() {
  auto *usuario = new CadastroUsuario(this);
  usuario->viewRegisterById(User::idUsuario);
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

  User::setSetting("User/servidorSMTP", ui->lineEditServidorSMTP->text());
  User::setSetting("User/portaSMTP", ui->lineEditPortaSMTP->text());
  User::setSetting("User/emailCompra", ui->lineEditEmail->text());
  User::setSetting("User/emailSenha", ui->lineEditEmailSenha->text());
  User::setSetting("User/emailCopia", ui->lineEditEmailCopia->text());

  auto *mail = new SendMail(SendMail::Tipo::Teste, this);
  mail->setAttribute(Qt::WA_DeleteOnClose);

  mail->show();
}
