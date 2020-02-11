#include "acbr.h"

#include "application.h"
#include "usersession.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>

ACBr::ACBr(QObject *parent) : QObject(parent) {
  connect(&socket, qOverload<QTcpSocket::SocketError>(&QAbstractSocket::error), this, &ACBr::error);
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
  qApp->enqueueError("Erro socket: " + socket.errorString());
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

bool ACBr::gerarDanfe(const int idNFe) {
  if (idNFe == 0) { return qApp->enqueueError(false, "Produto não possui nota!"); }

  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando chaveAcesso: " + query.lastError().text()); }

  return gerarDanfe(query.value("xml").toByteArray(), true).has_value();
}

std::optional<QString> ACBr::gerarDanfe(const QByteArray &fileContent, const bool openFile) {
  QFile file("xml.xml");

  if (not file.open(QFile::WriteOnly)) {
    qApp->enqueueError("Erro abrindo arquivo para escrita: " + file.errorString());
    return {};
  }

  file.write(fileContent);

  file.close();

  QFileInfo info(file);

  auto respostaSavePdf = enviarComando("NFE.ImprimirDANFEPDF(" + info.absoluteFilePath() + ")", true);

  if (not respostaSavePdf) { return {}; }

  if (not respostaSavePdf->contains("Arquivo criado em:")) {
    qApp->enqueueError(respostaSavePdf.value() + " - Verifique se o arquivo não está aberto!");
    return {};
  }

  respostaSavePdf = respostaSavePdf->remove("OK: Arquivo criado em: ");

  if (openFile) { abrirPdf(respostaSavePdf.value()); }

  return respostaSavePdf;

  // TODO: 1copiar arquivo para pasta predefinida e renomear arquivo para formato 'DANFE_xxx.xxx_idpedido'
}

std::optional<std::tuple<QString, QString>> ACBr::consultarNFe(const int idNFe) {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) {
    qApp->enqueueError("Erro buscando XML: " + query.lastError().text());
    return {};
  }

  const QString filePath = "C:/ACBrMonitorPLUS/nfe.xml";

  const auto resposta1 = enviarComando("NFE.SaveToFile(" + filePath + ", \"" + query.value("xml").toString() + "\")");

  if (not resposta1) { return {}; }

  qDebug() << "resposta1: " << resposta1.value();

  const auto resposta2 = enviarComando("NFE.ConsultarNFe(" + filePath + ")");

  if (not resposta2) { return {}; }

  qDebug() << "resposta2: " << resposta2.value();

  if (resposta2->contains("NF-e não consta na base de dados da SEFAZ")) {
    removerNota(idNFe);
    qApp->enqueueError("NFe não consta na SEFAZ, removendo do sistema...");
    return {};
  }

  if (not resposta2->contains("XMotivo=Autorizado o uso da NF-e") and not resposta2->contains("xEvento=Cancelamento registrado")) {
    qApp->enqueueError(resposta2.value());
    return {};
  }

  auto resposta3 = enviarComando("NFe.LoadFromFile(" + filePath + ")");

  if (not resposta3) { return {}; }

  qDebug() << "resposta3: " << resposta3.value();

  const QString xml = resposta3->remove("OK: ");

  return std::make_tuple<>(xml, resposta2.value());
}

void ACBr::removerNota(const int idNFe) {
  if (not qApp->startTransaction("ACBr::removerNota")) { return; }

  const bool remover = [&] {
    QSqlQuery query2a;
    query2a.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
    query2a.bindValue(":idNFeSaida", idNFe);

    if (not query2a.exec()) { return qApp->enqueueError(false, "Erro removendo nfe da venda: " + query2a.lastError().text()); }

    QSqlQuery query3a;
    query3a.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
    query3a.bindValue(":idNFeSaida", idNFe);

    if (not query3a.exec()) { return qApp->enqueueError(false, "Erro removendo nfe do veiculo: " + query3a.lastError().text()); }

    QSqlQuery queryNota;
    queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
    queryNota.bindValue(":idNFe", idNFe);

    if (not queryNota.exec()) { return qApp->enqueueError(false, "Erro removendo nota: " + queryNota.lastError().text()); }

    return true;
  }();

  if (not remover) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }
}

bool ACBr::abrirPdf(const QString &filePath) {
  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) { return qApp->enqueueError(false, "Erro abrindo PDF!"); }

  return true;
}

std::optional<QString> ACBr::enviarComando(const QString &comando, const bool local) {
  const auto servidorConfig = UserSession::getSetting("User/servidorACBr");
  const auto porta = UserSession::getSetting("User/portaACBr");

  if (not servidorConfig or not porta) {
    qApp->enqueueError("Preencher IP e porta do ACBr nas configurações!");
    return {};
  }

  recebido = false;
  enviado = false;
  resposta.clear();
  progressDialog->show();

  const auto servidor = local ? "localhost" : servidorConfig->toString();

  if (lastHost != servidor) { socket.disconnectFromHost(); }

  if (not conectado) {
    lastHost = servidor;

    socket.connectToHost(servidor, porta->toByteArray().toUShort());
  }

  while (not pronto) { QCoreApplication::processEvents(QEventLoop::AllEvents, 100); }

  socket.write(comando.toUtf8() + "\r\n.\r\n");

  while (not enviado and conectado) { QCoreApplication::processEvents(QEventLoop::AllEvents, 100); }

  while (not recebido and conectado) { QCoreApplication::processEvents(QEventLoop::AllEvents, 100); }

  progressDialog->cancel();

  return resposta;
}

bool ACBr::enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) {
  const auto respostaEmail = enviarComando("NFE.EnviarEmail(" + emailDestino + "," + filePath + ",1,'" + assunto + "', " + emailCopia + ")", true);

  if (not respostaEmail) { return false; }

  // TODO: perguntar se deseja tentar enviar novamente?
  if (not respostaEmail->contains("OK: Email enviado com sucesso")) { return qApp->enqueueError(false, respostaEmail.value()); }

  qApp->enqueueInformation(respostaEmail.value());

  return true;
}

// NOTE: se uma nota der erro na consulta o xml armazenado provavelmente está errado, nesses casos baixar o xml pelo DANFE ONLINE e substituir no sistema
