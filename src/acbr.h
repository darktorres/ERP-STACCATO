#pragma once

#include <QObject>
#include <QProgressDialog>
#include <QTcpSocket>

class ACBr final : public QObject {
  Q_OBJECT

public:
  explicit ACBr(QObject *parent = nullptr);
  ~ACBr() = default;

  auto consultarNFe(const int idNFe) -> std::tuple<QString, QString>;
  auto enviarComando(const QString &comando, const bool local = false) -> QString;
  auto enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) -> void;

private:
  // attributes
  bool conectado = false;
  bool enviado = false;
  bool pronto = false;
  bool recebido = false;
  QProgressDialog *progressDialog;
  QString const welcome = "Esperando por comandos.\x03";
  QString lastHost;
  QString resposta;
  QTcpSocket socket;
  // methods
  auto error(QAbstractSocket::SocketError socketError) -> void;
  auto readSocket() -> void;
  auto removerNota(const int idNFe) -> void;
  auto setConnected() -> void;
  auto setConnections() -> void;
  auto setDisconnected() -> void;
  auto write() -> void;
};
