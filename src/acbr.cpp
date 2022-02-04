#include "acbr.h"

#include "application.h"
#include "sqlquery.h"
#include "user.h"
#include "userconfig.h"

#include <QSqlError>
#include <QThread>
#include <QUrl>

ACBr::ACBr(QObject *parent) : QObject(parent) {
  progressDialog.reset();
  progressDialog.setCancelButton(nullptr);
  progressDialog.setLabelText("Esperando o emissor de NF-e ACBr...");
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
  case QAbstractSocket::SocketTimeoutError: throw RuntimeException("Erro conectando ao emissor de NF-e ACBr!\nVerifique se ele está aberto!");
  case QAbstractSocket::RemoteHostClosedError: socket.disconnectFromHost(); throw RuntimeException("Conexão com o emissor de NF-e ACBr encerrada!");
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

  if (not query.first()) { throw RuntimeException("XML não encontrado para a NF-e com id: '" + QString::number(idNFe) + "'"); }

  //-------------------------------------------

  const QString filePath = "C:/ACBrMonitorPLUS/nfe.xml";

  const QString respostaSalvar = enviarComando(R"(NFE.SaveToFile()" + filePath + R"(, ")" + query.value("xml").toString() + R"("))", "Salvando XML...");

  if (not respostaSalvar.contains("OK", Qt::CaseInsensitive)) { throw RuntimeException(respostaSalvar); }

  //  qDebug() << "respostaSalvar: " << respostaSalvar;

  //-------------------------------------------

  const QString respostaConsultar = enviarComando("NFE.ConsultarNFe(" + filePath + ")", "Consultando NF-e...");

  //  qDebug() << "respostaConsultar: " << respostaConsultar;

  if (respostaConsultar.contains("NF-e não consta na base de dados da SEFAZ", Qt::CaseInsensitive)) {
    removerNota(idNFe);
    throw RuntimeException("NF-e não consta na SEFAZ, removendo do sistema...");
  }

  if (not respostaConsultar.contains("XMotivo=Autorizado o uso da NF-e", Qt::CaseInsensitive) and not respostaConsultar.contains("xEvento=Cancelamento registrado", Qt::CaseInsensitive) and
      not respostaConsultar.contains("XMotivo=Uso Denegado", Qt::CaseInsensitive)) {
    throw RuntimeException(respostaConsultar);
  }

  //-------------------------------------------

  QString respostaCarregar = enviarComando("NFe.LoadFromFile(" + filePath + ")", "Carregando NF-e...");

  if (not respostaCarregar.contains("OK", Qt::CaseInsensitive)) { throw RuntimeException(respostaCarregar); }

  //  qDebug() << "respostaCarregar: " << respostaCarregar;

  const QString xml = respostaCarregar.remove("OK: ");

  return std::make_tuple<>(xml, respostaConsultar);
}

void ACBr::removerNota(const int idNFe) {
  qApp->startTransaction("ACBr::removerNota");

  SqlQuery query2a;
  query2a.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
  query2a.bindValue(":idNFeSaida", idNFe);

  if (not query2a.exec()) { throw RuntimeException("Erro removendo NF-e da venda: " + query2a.lastError().text()); }

  SqlQuery query3a;
  query3a.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
  query3a.bindValue(":idNFeSaida", idNFe);

  if (not query3a.exec()) { throw RuntimeException("Erro removendo NF-e do veiculo: " + query3a.lastError().text()); }

  SqlQuery queryNota;
  queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
  queryNota.bindValue(":idNFe", idNFe);

  if (not queryNota.exec()) { throw RuntimeException("Erro removendo NF-e: " + queryNota.lastError().text()); }

  qApp->endTransaction();
}

QString ACBr::enviarComando(const QString &comando, const QString &labelText) {
  recebido = false;
  enviado = false;
  resposta.clear();
  progressDialog.reset();

  if (servidor.isEmpty() and porta.isEmpty()) {
    SqlQuery queryConfig;

    if (not queryConfig.exec("SELECT servidorACBr, portaACBr FROM config")) { throw RuntimeException("Erro buscando dados do emissor de NF-e: " + queryConfig.lastError().text()); }

    if (not queryConfig.first() or queryConfig.value("servidorACBr").toString().isEmpty() or queryConfig.value("portaACBr").toString().isEmpty()) {
      auto *config = new UserConfig(nullptr);
      config->setAttribute(Qt::WA_DeleteOnClose);
      config->show();

      throw RuntimeError("Configure o emissor de NF-e primeiro!");
    }

    servidor = queryConfig.value("servidorACBr").toString();
    porta = queryConfig.value("portaACBr").toString();
  }

  if (not labelText.isEmpty()) { progressDialog.setLabelText(labelText); }
  if (not qApp->getSilent()) { progressDialog.show(); }

  if (not conectado) { socket.connectToHost(servidor, porta.toUShort()); }

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

  // TODO: caso o ACBr retorne alguma das mensagens abaixo pedir para o usuario remover e reconectar o leitor do certificado
  // Erro ao criar a chave do CSP.
  // Erro relacionado ao Canal Seguro

  return resposta;
}

void ACBr::enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) {
  Q_UNUSED(emailCopia)

  //    const QString respostaEmail = enviarComando("NFE.EnviarEmail(" + emailDestino + "," + filePath + ",1,'" + assunto + "', " + emailCopia + ")", true);
  const QString respostaEmail = enviarComando("NFE.EnviarEmail(" + emailDestino + "," + filePath + ",1,'" + assunto + "')", "Enviando e-mail...");

  // TODO: perguntar se deseja tentar enviar novamente?
  if (not respostaEmail.contains("OK: E-mail enviado com sucesso!", Qt::CaseInsensitive)) { throw RuntimeException(respostaEmail); }

  qApp->enqueueInformation(respostaEmail);
}

void ACBr::setarServidor(const QString &servidor_, const QString &porta_) {
  servidor = servidor_;
  porta = porta_;
}

// TODO: colocar parent em progressDialog para que a barra fique centralizada com a janela que chamou
