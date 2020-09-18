#include "acbr.h"

#include "application.h"
#include "usersession.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QThread>
#include <QUrl>

ACBr::ACBr() : QObject(), progressDialog(new QProgressDialog()) {
  connect(&socket, qOverload<QTcpSocket::SocketError>(&QAbstractSocket::error), this, &ACBr::error); // TODO: change in 5.15 to errorOcurred
  connect(&socket, &QTcpSocket::connected, this, &ACBr::setConnected);
  connect(&socket, &QTcpSocket::disconnected, this, &ACBr::setDisconnected);
  connect(&socket, &QTcpSocket::readyRead, this, &ACBr::readSocket);
  connect(&socket, &QTcpSocket::bytesWritten, this, &ACBr::write);

  progressDialog->reset();
  progressDialog->setCancelButton(nullptr);
  progressDialog->setLabelText("Esperando ACBr...");
  progressDialog->setWindowTitle("ERP Staccato");
  progressDialog->setWindowModality(Qt::WindowModal);
  progressDialog->setMaximum(0);
  progressDialog->setMinimum(0);
}

void ACBr::error() {
  const QString errorString = socket.errorString();

  if (errorString == "Connection refused" or errorString == "Connection timed out") {
    throw RuntimeException("Erro conectando ao ACBr! Verifique se ele está aberto!");
  } else {
    throw RuntimeException("Erro socket: " + socket.errorString());
  }

  progressDialog->cancel();
}

void ACBr::write() { enviado = true; }

void ACBr::setConnected() { conectado = true; }

void ACBr::setDisconnected() {
  pronto = false;
  conectado = false;
  recebido = false;
  enviado = false;
}

void ACBr::readSocket() {
  const auto stream = socket.readAll();
  resposta += stream;

  if (resposta.endsWith(welcome)) {
    resposta.clear();
    pronto = true;
    return;
  }

  if (resposta.endsWith("\u0003")) {
    resposta.remove("\u0003");
    recebido = true;
    return;
  }
}

void ACBr::gerarDanfe(const int idNFe) {
  if (idNFe == 0) { throw RuntimeError("Produto não possui nota!"); }

  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando chaveAcesso: " + query.lastError().text()); }

  gerarDanfe(query.value("xml").toByteArray(), true);
}

QString ACBr::gerarDanfe(const QByteArray &fileContent, const bool openFile) {
  if (fileContent.indexOf("Id=") == -1) { throw RuntimeException("Não encontrado a chave de acesso!"); }

  const QString chaveAcesso = fileContent.mid(fileContent.indexOf("Id=") + 7, 44);

  QFile file(QDir::currentPath() + "/arquivos/" + chaveAcesso + ".xml");

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString()); }

  file.write(fileContent);

  file.close();

  QFileInfo info(file);

  QString respostaSavePdf = enviarComando("NFE.ImprimirDANFEPDF(" + info.absoluteFilePath() + ")", true);

  if (not respostaSavePdf.contains("Arquivo criado em:")) { throw RuntimeException(respostaSavePdf + " - Verifique se o arquivo não está aberto!"); }

  respostaSavePdf = respostaSavePdf.remove("OK: Arquivo criado em: ");

  if (openFile) { abrirPdf(respostaSavePdf); }

  return respostaSavePdf;

  // TODO: 1copiar arquivo para pasta predefinida e renomear arquivo para formato 'DANFE_xxx.xxx_idpedido'
}

std::tuple<QString, QString> ACBr::consultarNFe(const int idNFe) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando XML: " + query.lastError().text()); }

  const QString filePath = "C:/ACBrMonitorPLUS/nfe.xml";

  const QString resposta1 = enviarComando("NFE.SaveToFile(" + filePath + ", \"" + query.value("xml").toString() + "\")");

  // TODO: validar resposta1

  qDebug() << "resposta1: " << resposta1;

  const QString resposta2 = enviarComando("NFE.ConsultarNFe(" + filePath + ")");

  qDebug() << "resposta2: " << resposta2;

  if (resposta2.contains("NF-e não consta na base de dados da SEFAZ")) {
    removerNota(idNFe);
    throw RuntimeException("NFe não consta na SEFAZ, removendo do sistema...");
  }

  if (not resposta2.contains("XMotivo=Autorizado o uso da NF-e") and not resposta2.contains("xEvento=Cancelamento registrado")) { throw RuntimeException(resposta2); }

  QString resposta3 = enviarComando("NFe.LoadFromFile(" + filePath + ")");

  // TODO: validar resposta3

  qDebug() << "resposta3: " << resposta3;

  const QString xml = resposta3.remove("OK: ");

  return std::make_tuple<>(xml, resposta2);
}

void ACBr::removerNota(const int idNFe) {
  qApp->startTransaction("ACBr::removerNota");

  QSqlQuery query2a;
  query2a.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
  query2a.bindValue(":idNFeSaida", idNFe);

  if (not query2a.exec()) { throw RuntimeException("Erro removendo nfe da venda: " + query2a.lastError().text()); }

  QSqlQuery query3a;
  query3a.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
  query3a.bindValue(":idNFeSaida", idNFe);

  if (not query3a.exec()) { throw RuntimeException("Erro removendo nfe do veiculo: " + query3a.lastError().text()); }

  QSqlQuery queryNota;
  queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
  queryNota.bindValue(":idNFe", idNFe);

  if (not queryNota.exec()) { throw RuntimeException("Erro removendo nota: " + queryNota.lastError().text()); }

  qApp->endTransaction();
}

void ACBr::abrirPdf(const QString &filePath) {
  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) { throw RuntimeException("Erro abrindo PDF!"); }
}

QString ACBr::enviarComando(const QString &comando, const bool local) {
  recebido = false;
  enviado = false;
  resposta.clear();
  progressDialog->reset();

  if (local) {
    if (not qApp->getSilent()) { progressDialog->show(); }

    if (not conectado) { socket.connectToHost("localhost", 3434); }
  }

  if (not local) {
    const QString servidorConfig = UserSession::getSetting("User/servidorACBr").toString();
    const QString porta = UserSession::getSetting("User/portaACBr").toString();

    if (servidorConfig.isEmpty() or porta.isEmpty()) { throw RuntimeError("Preencher IP e porta do ACBr nas configurações!"); }

    if (not qApp->getSilent()) { progressDialog->show(); }

    if (not conectado) { socket.connectToHost(servidorConfig, porta.toUShort()); }
  }

  while (not pronto) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog->wasCanceled()) { throw std::exception(); }
  }

  socket.write(comando.toUtf8() + "\r\n.\r\n");

  while (not enviado and conectado) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog->wasCanceled()) { throw std::exception(); }
  }

  while (not recebido and conectado) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog->wasCanceled()) { throw std::exception(); }
  }

  progressDialog->cancel();

  return resposta;
}

void ACBr::enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) {
  const QString respostaEmail = enviarComando("NFE.EnviarEmail(" + emailDestino + "," + filePath + ",1,'" + assunto + "', " + emailCopia + ")", true);

  // TODO: perguntar se deseja tentar enviar novamente?
  if (not respostaEmail.contains("OK: E-mail enviado com sucesso!")) { throw RuntimeException(respostaEmail); }

  qApp->enqueueInformation(respostaEmail);
}

// NOTE: se uma nota der erro na consulta o xml armazenado provavelmente está errado, nesses casos baixar o xml pelo DANFE ONLINE e substituir no sistema
