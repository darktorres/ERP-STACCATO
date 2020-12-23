#pragma once

#include <QObject>
#include <QProgressDialog>
#include <QTcpSocket>

class ACBr final : public QObject {
  Q_OBJECT

public:
  explicit ACBr();
  ~ACBr() = default;
  auto consultarNFe(const int idNFe) -> std::tuple<QString, QString>;
  auto enviarComando(const QString &comando, const bool local = false) -> QString;
  auto enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) -> void;
  auto gerarDanfe(const QByteArray &fileContent, const bool openFile = true) -> QString;
  auto gerarDanfe(const int idNFe) -> void;

private:
  // attributes
  QTcpSocket socket;
  const QString welcome = "Esperando por comandos.\x03";
  QString lastHost;
  QString resposta;
  bool pronto = false;
  bool conectado = false;
  bool enviado = false;
  bool recebido = false;
  QProgressDialog *progressDialog;
  // methods
  auto abrirPdf(const QString &filePath) -> void;
  auto error(QAbstractSocket::SocketError socketError) -> void;
  auto readSocket() -> void;
  auto removerNota(const int idNFe) -> void;
  auto setConnected() -> void;
  auto setDisconnected() -> void;
  auto write() -> void;
};
