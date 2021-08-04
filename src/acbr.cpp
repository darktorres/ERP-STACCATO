#include "acbr.h"

#include "application.h"
#include "sqlquery.h"
#include "user.h"

#include <QDesktopServices>
#include <QSqlError>
#include <QThread>
#include <QUrl>

ACBr::ACBr(QObject *parent) : QObject(parent) {
  progressDialog.reset();
  progressDialog.setCancelButton(nullptr);
  progressDialog.setLabelText("Esperando ACBr...");
  progressDialog.setWindowTitle("ERP Staccato");
  progressDialog.setWindowModality(Qt::WindowModal);
  progressDialog.setMaximum(0);
  progressDialog.setMinimum(0);

  setConnections();
}

void ACBr::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&socket, &QAbstractSocket::errorOccurred, this, &ACBr::error, connectionType);
  connect(&socket, &QTcpSocket::connected, this, &ACBr::setConnected, connectionType);
  connect(&socket, &QTcpSocket::disconnected, this, &ACBr::setDisconnected, connectionType);
  connect(&socket, &QTcpSocket::readyRead, this, &ACBr::readSocket, connectionType);
  connect(&socket, &QTcpSocket::bytesWritten, this, &ACBr::write, connectionType);
}

void ACBr::error(QAbstractSocket::SocketError socketError) {
  progressDialog.cancel();

  switch (socketError) {
  case QAbstractSocket::ConnectionRefusedError: [[fallthrough]];
  case QAbstractSocket::SocketTimeoutError: throw RuntimeException("Erro conectando ao ACBr! Verifique se ele está aberto!");
  case QAbstractSocket::RemoteHostClosedError: socket.disconnectFromHost(); throw RuntimeException("Conexão com ACBr encerrada!");
  default: throw RuntimeException("Erro socket: " + socket.errorString());
  }
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

std::tuple<QString, QString> ACBr::consultarNFe(const int idNFe) {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro buscando XML: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("XML não encontrado para a NFe com id: " + QString::number(idNFe)); }

  //-------------------------------------------

  const QString filePath = "C:/ACBrMonitorPLUS/nfe.xml";

  const QString respostaSalvar = enviarComando("NFE.SaveToFile(" + filePath + ", \"" + query.value("xml").toString() + "\")");

  if (not respostaSalvar.contains("OK")) { throw RuntimeException(respostaSalvar); }

  //  qDebug() << "respostaSalvar: " << respostaSalvar;

  //-------------------------------------------

  const QString respostaConsultar = enviarComando("NFE.ConsultarNFe(" + filePath + ")");

  //  qDebug() << "respostaConsultar: " << respostaConsultar;

  if (respostaConsultar.contains("NF-e não consta na base de dados da SEFAZ")) {
    removerNota(idNFe);
    throw RuntimeException("NFe não consta na SEFAZ, removendo do sistema...");
  }

  if (not respostaConsultar.contains("XMotivo=Autorizado o uso da NF-e") and not respostaConsultar.contains("xEvento=Cancelamento registrado") and
      not respostaConsultar.contains("XMotivo=Uso Denegado")) {
    throw RuntimeException(respostaConsultar);
  }

  //-------------------------------------------

  QString respostaCarregar = enviarComando("NFe.LoadFromFile(" + filePath + ")");

  if (not respostaCarregar.contains("OK")) { throw RuntimeException(respostaCarregar); }

  //  qDebug() << "respostaCarregar: " << respostaCarregar;

  const QString xml = respostaCarregar.remove("OK: ");

  return std::make_tuple<>(xml, respostaConsultar);
}

void ACBr::removerNota(const int idNFe) {
  qApp->startTransaction("ACBr::removerNota");

  SqlQuery query2a;
  query2a.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
  query2a.bindValue(":idNFeSaida", idNFe);

  if (not query2a.exec()) { throw RuntimeException("Erro removendo nfe da venda: " + query2a.lastError().text()); }

  SqlQuery query3a;
  query3a.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
  query3a.bindValue(":idNFeSaida", idNFe);

  if (not query3a.exec()) { throw RuntimeException("Erro removendo nfe do veiculo: " + query3a.lastError().text()); }

  SqlQuery queryNota;
  queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
  queryNota.bindValue(":idNFe", idNFe);

  if (not queryNota.exec()) { throw RuntimeException("Erro removendo nota: " + queryNota.lastError().text()); }

  qApp->endTransaction();
}

QString ACBr::enviarComando(const QString &comando, const bool local) {
  recebido = false;
  enviado = false;
  resposta.clear();
  progressDialog.reset();

  if (local) {
    if (not qApp->getSilent()) { progressDialog.show(); }

    if (not conectado) { socket.connectToHost("localhost", 3434); }
  }

  if (not local) {
    const QString servidorConfig = User::getSetting("User/servidorACBr").toString();
    const QString porta = User::getSetting("User/portaACBr").toString();

    if (servidorConfig.isEmpty() or porta.isEmpty()) { throw RuntimeError("Preencher IP e porta do ACBr nas configurações!"); }

    if (not qApp->getSilent()) { progressDialog.show(); }

    if (not conectado) { socket.connectToHost(servidorConfig, porta.toUShort()); }
  }

  while (not pronto) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog.wasCanceled()) { throw std::exception(); }
  }

  socket.write(comando.toUtf8() + "\r\n.\r\n");

  while (not enviado and conectado) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog.wasCanceled()) { throw std::exception(); }
  }

  while (not recebido and conectado) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog.wasCanceled()) { throw std::exception(); }
  }

  progressDialog.cancel();

  return resposta;
}

void ACBr::enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) {
  const QString respostaEmail = enviarComando("NFE.EnviarEmail(" + emailDestino + "," + filePath + ",1,'" + assunto + "', " + emailCopia + ")", true);

  // TODO: perguntar se deseja tentar enviar novamente?
  if (not respostaEmail.contains("OK: E-mail enviado com sucesso!")) { throw RuntimeException(respostaEmail); }

  qApp->enqueueInformation(respostaEmail);
}
