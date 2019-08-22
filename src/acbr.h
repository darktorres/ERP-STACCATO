#pragma once

#include <QObject>
#include <QProgressDialog>
#include <QTcpSocket>

class ACBr final : public QObject {
  Q_OBJECT

public:
  explicit ACBr(QObject *parent = nullptr);
  ~ACBr() = default;
  auto consultarNFe(const int idNFe) -> std::optional<std::tuple<QString, QString>>;
  auto enviarComando(const QString &comando, const bool local = false) -> std::optional<QString>;
  auto enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &filePath) -> bool;
  auto gerarDanfe(const QByteArray &fileContent, const bool openFile = true) -> std::optional<QString>;
  auto gerarDanfe(const int idNFe) -> bool;

private:
  // attributes
  QTcpSocket socket;
  const QString welcome = "Esperando por comandos.\x03";
  QString resposta;
  bool pronto = false;
  bool conectado = false;
  bool enviado = false;
  bool recebido = false;
  QProgressDialog *progressDialog = new QProgressDialog();
  // methods
  auto abrirPdf(const QString &filePath) -> bool;
  auto error() -> void;
  auto readSocket() -> void;
  auto removerNota(const int idNFe) -> void;
  auto setConnected() -> void;
  auto setDisconnected() -> void;
  auto write() -> void;
};
