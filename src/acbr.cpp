#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

#include "acbr.h"
#include "usersession.h"

bool ACBr::gerarDanfe(const int idNFe) {
  if (idNFe == 0) {
    QMessageBox::critical(nullptr, "Erro!", "Produto não possui nota!");
    return false;
  }

  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(nullptr, "Erro!", "Erro buscando chaveAcesso: " + query.lastError().text());
    return false;
  }

  return gerarDanfe(query.value("xml").toByteArray(), true).has_value();
}

std::optional<QString> ACBr::gerarDanfe(const QByteArray &fileContent, const bool openFile) {
  const auto respostaSaveXml = enviarComando(R"(NFE.SaveToFile(xml.xml,")" + fileContent + R"(")");

  if (not respostaSaveXml) { return {}; }

  if (not respostaSaveXml->contains("OK")) {
    QMessageBox::critical(nullptr, "Erro!", "Erro salvando XML: " + *respostaSaveXml);
    return {};
  }

  auto respostaSavePdf = enviarComando("NFE.ImprimirDANFEPDF(xml.xml)");

  if (not respostaSavePdf) { return {}; }

  if (not respostaSavePdf->contains("Arquivo criado em:")) {
    QMessageBox::critical(nullptr, "Erro!", *respostaSavePdf);
    QMessageBox::critical(nullptr, "Erro!", "Verifique se o arquivo não está aberto!");
    return {};
  }

  respostaSavePdf = respostaSavePdf->remove("OK: Arquivo criado em: ");

  if (openFile) { abrirPdf(*respostaSavePdf); }

  return respostaSavePdf;

  // TODO: 1copiar arquivo para pasta predefinida e renomear arquivo para formato 'DANFE_xxx.xxx_idpedido'
}

std::optional<std::tuple<QString, QString>> ACBr::consultarNFe(const int idNFe) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(nullptr, "Erro!", "Erro buscando XML: " + query.lastError().text());
    return {};
  }

  QFile file(QDir::currentPath() + "/temp.xml");

  if (not file.open(QFile::WriteOnly)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro abrindo arquivo para escrita: " + file.errorString());
    return {};
  }

  QTextStream stream(&file);

  stream << query.value("xml").toString();

  file.close();

  auto resposta = ACBr::enviarComando("NFE.ConsultarNFe(" + file.fileName() + ")");

  if (not resposta) { return {}; }

  if (not resposta->contains("XMotivo=Autorizado o uso da NF-e")) {
    QMessageBox::critical(nullptr, "Resposta ConsultarNFe: ", *resposta);
    return {};
  }

  QFile newFile(QDir::currentPath() + "/temp.xml");

  if (not newFile.open(QFile::ReadOnly)) {
    QMessageBox::critical(nullptr, "Erro!", "Erro abrindo arquivo para leitura: " + newFile.errorString());
    return {};
  }

  const QString xml = newFile.readAll();

  return std::make_tuple<>(xml, *resposta);
}

bool ACBr::abrirPdf(const QString &resposta) {
  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(resposta))) {
    QMessageBox::critical(nullptr, "Erro!", "Erro abrindo PDF!");
    return false;
  }

  return true;
}

std::optional<QString> ACBr::enviarComando(const QString &comando) {
  if (socket->state() != QTcpSocket::ConnectedState) {
    const auto servidor = UserSession::getSetting("User/servidorACBr");
    const auto ip = UserSession::getSetting("User/portaACBr");

    if (not servidor or not ip) {
      QMessageBox::critical(nullptr, "Erro!", "Preencher IP e porta do ACBr nas configurações!");
      return {};
    }

    socket->connectToHost(servidor.value().toString(), ip.value().toByteArray().toUShort());

    if (not socket->waitForConnected(5000)) {
      QMessageBox::critical(nullptr, "Erro!", "Não foi possível conectar ao ACBr: " + socket->errorString());
      return {};
    }

    socket->waitForReadyRead();

    socket->readAll(); // lendo mensagem de boas vindas
  }

  socket->write(comando.toUtf8() + "\r\n.\r\n");
  socket->waitForBytesWritten();

  auto *progressDialog = new QProgressDialog(nullptr);
  progressDialog->reset();
  progressDialog->setCancelButton(nullptr);
  progressDialog->setLabelText("Esperando ACBr...");
  progressDialog->setWindowTitle("ERP Staccato");
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setMaximum(0);
  progressDialog->setMinimum(0);
  progressDialog->show();

  connect(socket, &QTcpSocket::readyRead, progressDialog, &QProgressDialog::cancel);

  while (not progressDialog->wasCanceled()) { QCoreApplication::processEvents(QEventLoop::AllEvents, 100); }

  const QString resposta = QString(socket->readAll()).remove("\u0003");

  if (resposta.isEmpty()) {
    QMessageBox::critical(nullptr, "Erro!", "Não obteve resposta do ACBr!");
    return {};
  }

  return resposta;
}

bool ACBr::enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &anexo) {
  const auto resposta = ACBr::enviarComando("NFE.EnviarEmail(" + emailDestino + "," + anexo + ",1,'" + assunto + "', " + emailCopia + ")");

  if (not resposta) { return false; }

  // TODO: perguntar se deseja tentar enviar novamente?
  if (not resposta->contains("OK: Email enviado com sucesso")) {
    QMessageBox::critical(nullptr, "Erro!", *resposta);
    return false;
  }

  QMessageBox::information(nullptr, "Aviso!", *resposta);

  return true;
}
