#include <QFileDialog>
#include <QSqlError>

#include "sendmail.h"
#include "smtp.h"
#include "ui_sendmail.h"
#include "usersession.h"

SendMail::SendMail(const Tipo tipo, const QString &arquivo, const QString &fornecedor, QWidget *parent) : QDialog(parent), fornecedor(fornecedor), tipo(tipo), ui(new Ui::SendMail) {
  // TODO: 5colocar arquivo como vetor de strings para multiplos anexos
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  files.append(arquivo);

  ui->lineEditAnexo->setText(arquivo);

  if (tipo == Tipo::GerarCompra) {
    const QFileInfo info(arquivo);

    ui->lineEditTitulo->setText("PEDIDO " + info.baseName());

    QSqlQuery query;
    query.prepare("SELECT email, contatoNome FROM fornecedor WHERE razaoSocial = :razaoSocial");
    query.bindValue(":razaoSocial", fornecedor);

    if (not query.exec()) QMessageBox::critical(this, "Erro!", "Erro buscando email do fornecedor: " + query.lastError().text());

    QString representante;

    if (query.first()) {
      representante = query.value("contatoNome").toString();
      QStringList list = representante.split(" ");

      for (auto &nome : list) {
        nome = nome.toLower();
        nome[0] = nome[0].toUpper();
      }

      representante = list.join(" ");

      do {
        ui->comboBoxDest->addItem(query.value("email").toString());
      } while (query.next());
    }

    // REFAC: 5dont hardcode this
    // REFAC:__project public code
    ui->textEdit->setHtml(
        R"(<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0//EN" "http://www.w3.org/TR/REC-html40/strict.dtd"><html><head><meta name="qrichtext" content="1" /><style type="text/css">p, li { white-space: pre-wrap; }</style></head><body style=" font-family:'MS Shell Dlg 2'; font-size:8pt; font-weight:400; font-style:normal;"><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;">)" +
        QString(QTime::currentTime().hour() > 12 ? "Boa tarde" : "Bom dia") + " prezado(a) " + (representante.isEmpty() ? "parceiro(a)" : representante) +
        R"(;</span></p><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;"><span style=" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;">Segue pedido, aguardo espelho como confirmação e previsão de disponibilidade/ coleta do mesmo.</span></p><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">Grato!</p><p style=" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;">   Att.</p></body></html>)");

    ui->textEdit->document()->addResource(QTextDocument::ImageResource, QUrl("cid:assinatura.png@gmail.com"), QImage("://assinatura conrado.png"));
    ui->textEdit->append(R"(<img src="cid:assinatura.png@gmail.com" />)");
  }

  if (tipo == Tipo::CancelarNFe) {
    ui->comboBoxDest->addItem(UserSession::getSetting("User/emailContabilidade").toString());

    ui->lineEditTitulo->setText("CANCELAMENTO DE NFe - STACCATO REVESTIMENTOS COMERCIO E REPRESENTACAO LTDA");

    // corpo email ...

    ui->textEdit->setText("Cancelamento da NFe de numero 1234...");
  }

  if (tipo != Tipo::Vazio) {
    ui->lineEditEmail->setText(UserSession::getSetting("User/emailCompra").toString());
    ui->lineEditCopia->setText(UserSession::getSetting("User/emailCopia").toString());
    ui->lineEditServidor->setText(UserSession::getSetting("User/servidorSMTP").toString());
    ui->lineEditPorta->setText(UserSession::getSetting("User/portaSMTP").toString());
    ui->lineEditPasswd->setText(UserSession::getSetting("User/emailSenha").toString());
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

  for (const auto &file : files) fileListString.append(R"(")" + QFileInfo(file).fileName() + R"(" )");

  ui->lineEditAnexo->setText(fileListString);
}

void SendMail::on_pushButtonEnviar_clicked() {
  progress->show();

  UserSession::setSetting("User/servidorSMTP", ui->lineEditServidor->text());
  UserSession::setSetting("User/portaSMTP", ui->lineEditPorta->text());
  UserSession::setSetting("User/emailCompra", ui->lineEditEmail->text());
  UserSession::setSetting("User/emailSenha", ui->lineEditPasswd->text());

  Smtp *smtp = new Smtp(ui->lineEditEmail->text(), ui->lineEditPasswd->text(), ui->lineEditServidor->text(), ui->lineEditPorta->text().toInt());
  connect(smtp, &Smtp::status, this, &SendMail::mailSent);

  smtp->sendMail(ui->lineEditEmail->text(), ui->comboBoxDest->currentText(), ui->lineEditCopia->text(), ui->lineEditTitulo->text(), ui->textEdit->toHtml(), files,
                 tipo == Tipo::GerarCompra ? "://assinatura conrado.png" : "");
}

void SendMail::mailSent(const QString &status) {
  progress->cancel();
  status == "Message sent" ? successStatus() : failureStatus(status);
}

void SendMail::successStatus() {
  QMessageBox::information(nullptr, tr("Qt Simple SMTP client"), tr("Mensagem enviada!"));

  QDialog::accept();
}

void SendMail::failureStatus(const QString &status) { QMessageBox::critical(nullptr, tr("Qt Simple SMTP client"), "Ocorreu erro: " + status); }
