#include "sendmail.h"
#include "ui_sendmail.h"

#include "application.h"
#include "smtp.h"
#include "usersession.h"

#include <QFileDialog>
#include <QSqlError>

SendMail::SendMail(const Tipo tipo, const QString &arquivo, const QString &fornecedor, QWidget *parent) : QDialog(parent), fornecedor(fornecedor), tipo(tipo), ui(new Ui::SendMail) {
  // TODO: 5colocar arquivo como vetor de strings para multiplos anexos
  ui->setupUi(this);

  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &SendMail::on_pushButtonBuscar_clicked);
  connect(ui->pushButtonEnviar, &QPushButton::clicked, this, &SendMail::on_pushButtonEnviar_clicked);

  setWindowFlags(Qt::Window);

  files.append(arquivo);

  ui->lineEditAnexo->setText(arquivo);

  if (tipo == Tipo::GerarCompra) {
    const QFileInfo info(arquivo);

    ui->lineEditTitulo->setText("PEDIDO " + info.baseName());

    QSqlQuery query;
    query.prepare("SELECT email, contatoNome FROM fornecedor WHERE razaoSocial = :razaoSocial");
    query.bindValue(":razaoSocial", fornecedor);

    if (not query.exec()) { qApp->enqueueError("Erro buscando email do fornecedor: " + query.lastError().text(), this); }

    QString representante;

    if (query.first()) {
      representante = query.value("contatoNome").toString();
      QStringList list = representante.split(" ");

      for (auto &nome : list) {
        nome = nome.toLower();
        nome[0] = nome[0].toUpper();
      }

      representante = list.join(" ");

      do { ui->comboBoxDest->addItem(query.value("email").toString()); } while (query.next());
    }

    // REFAC: 5dont hardcode this
    // REFAC:__project public code
    ui->textEdit->setHtml(
        R"(<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"><html><head><meta name="qrichtext" content="1" /><style type="text/css">p, li { white-space: pre-wrap; }</style></head><body style=" font-family:Calibri, sans-serif; font-size:11pt; font-weight:400; font-style:normal;"><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-family:Calibri, sans-serif; font-size:11pt; font-weight:400; font-style:normal;">)" +
        QString(QTime::currentTime().hour() > 12 ? "Boa tarde" : "Bom dia") + " prezado(a) " + (representante.isEmpty() ? "parceiro(a)" : representante) +
        R"(;</span></p><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-family:Calibri, sans-serif; font-size:11pt; font-weight:400; font-style:normal;">Segue pedido, aguardo espelho como confirmação e previsão de disponibilidade/ coleta do mesmo.</span></p><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style="font-family: Calibri, sans-serif; font-size: 11pt; font-weight: 400; font-style: normal;">Grato!</p><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style="font-family: Calibri, sans-serif; font-size: 11pt; font-weight: 400; font-style: normal;">   Att.</p></body></html>)");

    ui->textEdit->document()->addResource(QTextDocument::ImageResource, QUrl("cid:assinatura.png@gmail.com"), QImage("://assinatura conrado.png"));
    ui->textEdit->append(R"(<img src="cid:assinatura.png@gmail.com" />)");
  }

  if (tipo != Tipo::Vazio) {
    if (const auto key = UserSession::getSetting("User/emailCompra")) { ui->lineEditEmail->setText(key->toString()); }
    if (const auto key = UserSession::getSetting("User/emailCopia")) { ui->lineEditCopia->setText(key->toString()); }
    if (const auto key = UserSession::getSetting("User/servidorSMTP")) { ui->lineEditServidor->setText(key->toString()); }
    if (const auto key = UserSession::getSetting("User/portaSMTP")) { ui->lineEditPorta->setText(key->toString()); }
    if (const auto key = UserSession::getSetting("User/emailSenha")) { ui->lineEditPasswd->setText(key->toString()); }
  }

  progress = new QProgressDialog("Enviando...", "Cancelar", 0, 0, this);
  progress->setCancelButton(nullptr);
  progress->reset();
}

SendMail::~SendMail() { delete ui; }

void SendMail::on_pushButtonBuscar_clicked() {
  files.clear();

  files = QFileDialog::getOpenFileNames(this, "Abrir anexos", QDir::homePath());

  QString fileListString;

  for (const auto &file : std::as_const(files)) { fileListString.append(R"(")" + QFileInfo(file).fileName() + R"(" )"); }

  ui->lineEditAnexo->setText(fileListString);
}

void SendMail::on_pushButtonEnviar_clicked() {
  progress->show();

  UserSession::setSetting("User/servidorSMTP", ui->lineEditServidor->text());
  UserSession::setSetting("User/portaSMTP", ui->lineEditPorta->text());
  UserSession::setSetting("User/emailCompra", ui->lineEditEmail->text());
  UserSession::setSetting("User/emailSenha", ui->lineEditPasswd->text());

  Smtp *smtp = new Smtp(ui->lineEditEmail->text(), ui->lineEditPasswd->text(), ui->lineEditServidor->text(), ui->lineEditPorta->text().toUShort());
  connect(smtp, &Smtp::status, this, &SendMail::mailSent);

  smtp->sendMail(ui->lineEditEmail->text(), ui->comboBoxDest->currentText(), ui->lineEditCopia->text(), ui->lineEditTitulo->text(), ui->textEdit->toHtml(), files,
                 tipo == Tipo::GerarCompra ? "://assinatura conrado.png" : "");
}

void SendMail::mailSent(const QString &status) {
  progress->cancel();
  (status == "Message sent") ? successStatus() : failureStatus(status);
}

void SendMail::successStatus() {
  qApp->enqueueInformation("Mensagem enviada!", this);

  QDialog::accept();
}

void SendMail::failureStatus(const QString &status) { qApp->enqueueError("Ocorreu erro: " + status, this); }
