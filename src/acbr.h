#ifndef ACBR_H
#define ACBR_H

#include <QObject>
#include <QTcpSocket>
#include <optional>

class ACBr final : public QObject {
  Q_OBJECT

public:
  ACBr() = delete;
  static auto consultarNFe(const int idNFe) -> std::optional<std::tuple<QString, QString>>;
  static auto enviarComando(const QString &comando) -> std::optional<QString>;
  static auto enviarEmail(const QString &emailDestino, const QString &emailCopia, const QString &assunto, const QString &anexo) -> bool;
  static auto gerarDanfe(const QByteArray &fileContent, const bool openFile = true) -> std::optional<QString>;
  static auto gerarDanfe(const int idNFe) -> bool;

private:
  // attributes
  inline static QTcpSocket *socket = new QTcpSocket(nullptr);
  // methods
  static auto abrirPdf(const QString &resposta) -> bool;
};

#endif // ACBR_H
