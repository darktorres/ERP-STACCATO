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

ACBr::ACBr(QObject *parent) : QObject(parent) {
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

ACBr::ACBr() : ACBr(nullptr) {}

void ACBr::error() {
  const QString errorString = socket.errorString();

  if (errorString == "Connection refused" or errorString == "Connection timed out") {
    qApp->enqueueException("Erro conectando ao ACBr! Verifique se ele está aberto!");
  } else {
    qApp->enqueueException("Erro socket: " + socket.errorString());
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

bool ACBr::gerarDanfe(const int idNFe) {
  if (idNFe == 0) { return qApp->enqueueError(false, "Produto não possui nota!"); }

  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", idNFe);

  if (not query.exec() or not query.first()) { return qApp->enqueueException(false, "Erro buscando chaveAcesso: " + query.lastError().text()); }

  return gerarDanfe(query.value("xml").toByteArray(), true).has_value();
}

std::optional<QString> ACBr::gerarDanfe(const QByteArray &fileContent, const bool openFile) {
  if (fileContent.indexOf("Id=") == -1) {
    qApp->enqueueException("Não encontrado a chave de acesso!");
    return {};
  }

  const QString chaveAcesso = fileContent.mid(fileContent.indexOf("Id=") + 7, 44);

  QFile file(QDir::currentPath() + "/arquivos/" + chaveAcesso + ".xml");

  if (not file.open(QFile::WriteOnly)) {
    qApp->enqueueException("Erro abrindo arquivo para escrita: " + file.errorString());
    return {};
  }

  file.write(fileContent);

  file.close();

  QFileInfo info(file);

  auto respostaSavePdf = enviarComando("NFE.ImprimirDANFEPDF(" + info.absoluteFilePath() + ")", true);

  if (not respostaSavePdf) { return {}; }

  if (not respostaSavePdf->contains("Arquivo criado em:")) {
    qApp->enqueueException(respostaSavePdf.value() + " - Verifique se o arquivo não está aberto!");
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
    qApp->enqueueException("Erro buscando XML: " + query.lastError().text());
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
    qApp->enqueueException("NFe não consta na SEFAZ, removendo do sistema...");
    return {};
  }

  if (not resposta2->contains("XMotivo=Autorizado o uso da NF-e") and not resposta2->contains("xEvento=Cancelamento registrado")) {
    qApp->enqueueException(resposta2.value());
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

    if (not query2a.exec()) { return qApp->enqueueException(false, "Erro removendo nfe da venda: " + query2a.lastError().text()); }

    QSqlQuery query3a;
    query3a.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGA AGEND.', idNFeSaida = NULL WHERE idNFeSaida = :idNFeSaida");
    query3a.bindValue(":idNFeSaida", idNFe);

    if (not query3a.exec()) { return qApp->enqueueException(false, "Erro removendo nfe do veiculo: " + query3a.lastError().text()); }

    QSqlQuery queryNota;
    queryNota.prepare("DELETE FROM nfe WHERE idNFe = :idNFe");
    queryNota.bindValue(":idNFe", idNFe);

    if (not queryNota.exec()) { return qApp->enqueueException(false, "Erro removendo nota: " + queryNota.lastError().text()); }

    return true;
  }();

  if (not remover) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }
}

bool ACBr::abrirPdf(const QString &filePath) {
  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) { return qApp->enqueueException(false, "Erro abrindo PDF!"); }

  return true;
}

std::optional<QString> ACBr::enviarComando(const QString &comando, const bool local) {
  recebido = false;
  enviado = false;
  resposta.clear();
  progressDialog->reset();

  if (local) {
    progressDialog->show();

    if (not conectado) { socket.connectToHost("localhost", 3434); }
  }

  if (not local) {
    const auto servidorConfig = UserSession::getSetting("User/servidorACBr");
    const auto porta = UserSession::getSetting("User/portaACBr");

    if (not servidorConfig or not porta) {
      qApp->enqueueError("Preencher IP e porta do ACBr nas configurações!");
      return {};
    }

    progressDialog->show();

    if (not conectado) { socket.connectToHost(servidorConfig->toString(), porta->toByteArray().toUShort()); }
  }

  while (not pronto) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog->wasCanceled()) { return {}; }
  }

  socket.write(comando.toUtf8() + "\r\n.\r\n");

  while (not enviado and conectado) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog->wasCanceled()) { return {}; }
  }

  while (not recebido and conectado) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QThread::msleep(10);
    if (progressDialog->wasCanceled()) { return {}; }
  }

  progressDialog->cancel();

  return resposta;
}

bool ACBr::enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) {
  const auto respostaEmail = enviarComando("NFE.EnviarEmail(" + emailDestino + "," + filePath + ",1,'" + assunto + "', " + emailCopia + ")", true);

  if (not respostaEmail) { return false; }

  // TODO: perguntar se deseja tentar enviar novamente?
  if (not respostaEmail->contains("OK: Email enviado com sucesso")) { return qApp->enqueueException(false, respostaEmail.value()); }

  qApp->enqueueInformation(respostaEmail.value());

  return true;
}

// NOTE: se uma nota der erro na consulta o xml armazenado provavelmente está errado, nesses casos baixar o xml pelo DANFE ONLINE e substituir no sistema
