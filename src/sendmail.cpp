#include "sendmail.h"
#include "ui_sendmail.h"

#include "application.h"
#include "file.h"
#include "smtp.h"
#include "user.h"

#include <QAuthenticator>
#include <QFileDialog>
#include <QSqlError>

SendMail::SendMail(const Tipo tipo, const QString &arquivo, const QString &fornecedor, QWidget *parent) : QDialog(parent), fornecedor(fornecedor), tipo(tipo), ui(new Ui::SendMail) {
  // TODO: 5colocar arquivo como vetor de strings para multiplos anexos
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  files.append(arquivo);

  ui->lineEditAnexo->setText(arquivo);

  if (tipo != Tipo::Vazio) {
    SqlQuery query;

    if (not query.exec("SELECT servidorEmail, portaEmail, email, senhaEmail, copiaParaEmail FROM usuario_has_config WHERE idUsuario = " + User::idUsuario)) {
      throw RuntimeException("Erro buscando dados do email: " + query.lastError().text());
    }

    if (query.first()) {
      ui->lineEditServidor->setText(query.value("servidorEmail").toString());
      ui->lineEditPorta->setText(query.value("portaEmail").toString());
      ui->lineEditEmail->setText(query.value("email").toString());
      ui->lineEditPasswd->setText(query.value("senhaEmail").toString());
      ui->lineEditCopia->setText(query.value("copiaParaEmail").toString());
    }
  }

  if (tipo == Tipo::Teste) {
    ui->comboBoxDest->setCurrentText(ui->lineEditEmail->text());
    ui->lineEditTitulo->setText("Teste");
    ui->textEdit->setText("Mensagem de teste");
  }

  if (tipo == Tipo::GerarCompra) {
    const QFileInfo info(arquivo);

    ui->lineEditTitulo->setText("PEDIDO " + info.baseName());

    SqlQuery query;
    query.prepare("SELECT email, contatoNome FROM fornecedor WHERE razaoSocial = :razaoSocial");
    query.bindValue(":razaoSocial", fornecedor);

    if (not query.exec()) { throw RuntimeException("Erro buscando email do fornecedor: " + query.lastError().text(), this); }

    QString representante;

    if (query.first()) {
      representante = query.value("contatoNome").toString();
      QStringList list = representante.split(" ");

      for (auto &nome : list) {
        nome = nome.toLower();
        nome[0] = nome[0].toUpper();
      }

      representante = list.join(" ");

      // TODO: porque tem um loop aqui se vai ter apenas um resultado na query?
      do { ui->comboBoxDest->addItem(query.value("email").toString()); } while (query.next());
    }

    if (representante.isEmpty()) { representante = "parceiro(a)"; }

    QString diaTarde = QString(QTime::currentTime().hour() > 12 ? "Boa tarde" : "Bom dia");

    SqlQuery query2;
    query2.prepare("SELECT mensagemEmail FROM usuario_has_config WHERE idUsuario = :idUsuario");
    query2.bindValue(":idUsuario", User::idUsuario);

    if (not query2.exec()) { throw RuntimeException("Erro buscando assinatura: " + query2.lastError().text()); }

    if (query2.first()) {
      QString mensagem = query2.value("mensagemEmail").toString();
      mensagem.replace("{Bom dia}", diaTarde);
      mensagem.replace("{representante}", representante);
      ui->textEdit->setHtml(mensagem);
    }
  }

  SqlQuery query2;
  query2.prepare("SELECT assinaturaEmail FROM usuario_has_config WHERE idUsuario = :idUsuario");
  query2.bindValue(":idUsuario", User::idUsuario);

  if (not query2.exec()) { throw RuntimeException("Erro buscando assinatura: " + query2.lastError().text()); }

  if (query2.first()) {
    const QString url = query2.value("assinaturaEmail").toString();

    if (not url.isEmpty()) {
      auto *manager = new QNetworkAccessManager(this);
      manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

      connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
        Q_UNUSED(reply)

        authenticator->setUser(User::usuario);
        authenticator->setPassword(User::senha);
      });

      auto *reply = manager->get(QNetworkRequest(QUrl(url)));

      connect(reply, &QNetworkReply::finished, this, [=, this] {
        if (reply->error() != QNetworkReply::NoError) {
          if (reply->error() == QNetworkReply::ContentNotFoundError) { throw RuntimeError("Arquivo não encontrado no servidor!"); }

          throw RuntimeException("Erro ao baixar arquivo: " + reply->errorString(), this);
        }

        const auto replyMsg = reply->readAll();

        if (replyMsg.contains("could not be found on this server")) { throw RuntimeException("Arquivo não encontrado no servidor!"); }

        const QString assinaturaFilepath = QDir::currentPath() + "/assinaturas/" + User::usuario + ".png";

        File file(assinaturaFilepath);

        if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString(), this); }

        file.write(replyMsg);

        file.close();

        assinatura = assinaturaFilepath;
        qDebug() << "A: " << assinatura;
        ui->textEdit->document()->addResource(QTextDocument::ImageResource, QUrl("cid:assinatura.png@gmail.com"), QImage(assinaturaFilepath));
        ui->textEdit->append(R"(<img src="cid:assinatura.png@gmail.com" />)");
      });
    }
  }

  progress = new QProgressDialog("Enviando...", "Cancelar", 0, 0, this);
  progress->setCancelButton(nullptr);
  progress->reset();

  setConnections();
}

SendMail::~SendMail() { delete ui; }

void SendMail::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &SendMail::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->pushButtonEnviar, &QPushButton::clicked, this, &SendMail::on_pushButtonEnviar_clicked, connectionType);
}

void SendMail::on_pushButtonBuscar_clicked() {
  files.clear();

  files = QFileDialog::getOpenFileNames(this, "Abrir anexos", QDir::homePath());

  QString fileListString;

  for (const auto &file : std::as_const(files)) { fileListString.append(R"(")" + QFileInfo(file).fileName() + R"(" )"); }

  ui->lineEditAnexo->setText(fileListString);
}

void SendMail::on_pushButtonEnviar_clicked() {
  progress->show();

  User::setSetting("User/servidorSMTP", ui->lineEditServidor->text());
  User::setSetting("User/portaSMTP", ui->lineEditPorta->text());
  User::setSetting("User/emailCompra", ui->lineEditEmail->text());
  User::setSetting("User/emailSenha", ui->lineEditPasswd->text());

  Smtp *smtp = new Smtp(ui->lineEditEmail->text(), ui->lineEditPasswd->text(), ui->lineEditServidor->text(), ui->lineEditPorta->text().toUShort());
  connect(smtp, &Smtp::status, this, &SendMail::mailSent);

  smtp->sendMail(ui->lineEditEmail->text(), ui->comboBoxDest->currentText(), ui->lineEditCopia->text(), ui->lineEditTitulo->text(), ui->textEdit->toHtml(), files, assinatura);
}

void SendMail::mailSent(const QString &status) {
  progress->cancel();
  (status == "Message sent") ? successStatus() : failureStatus(status);
}

void SendMail::successStatus() {
  qApp->enqueueInformation("Mensagem enviada!", this);

  QDialog::accept();
}

void SendMail::failureStatus(const QString &status) { throw RuntimeException("Ocorreu erro: " + status, this); }
