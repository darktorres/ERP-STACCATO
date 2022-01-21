#pragma once

#include <QObject>
#include <QProgressDialog>
#include <QTcpSocket>

class ACBr final : public QObject {
  Q_OBJECT

public:
  explicit ACBr(QObject *parent = nullptr);
  ~ACBr() final = default;

  auto consultarNFe(const int idNFe) -> std::tuple<QString, QString>;
  auto enviarComando(const QString &comando) -> QString;
  auto enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) -> void;
  auto setarServidor(const QString &servidor_, const QString &porta_) -> void;

private:
  // attributes
  bool conectado = false;
  bool enviado = false;
  bool pronto = false;
  bool recebido = false;
  QProgressDialog progressDialog;
  QString const welcome = "Esperando por comandos.\x03";
  QString resposta;
  QString servidor;
  QString porta;
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
