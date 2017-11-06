#ifndef ACBR_H
#define ACBR_H

#include <QObject>
#include <QTcpSocket>

class ACBr : public QObject {
  Q_OBJECT

public:
  explicit ACBr(QObject *parent = 0);
  static bool gerarDanfe(const int idNFe);
  static std::optional<QString> enviarComando(const QString &comando);
  static std::optional<QString> gerarDanfe(const QByteArray &fileContent, const bool openFile = true);
  static std::optional<std::tuple<QString, QString>> consultarNFe(const int idNFe);

private:
  inline static QTcpSocket *socket = new QTcpSocket();
  static bool abrirPdf(const QString &resposta);
};

#endif // ACBR_H
