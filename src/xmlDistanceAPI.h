#ifndef XML_H
#define XML_H

#include <QDateTime>
#include <QDomElement>
#include <QStandardItemModel>

class XML_Distance final {

public:
  XML_Distance(const QByteArray &fileContent, const int evento, const int idVeiculo, const QString &modelo, const QDateTime &dataEvento);
  auto lerValores(const QStandardItem *item) -> void;

  QVector<QStringList> legs;
  QString distancia;
  QString startAddr;
  QString endAddr;
  QString duracao;
  const int evento;
  const int idVeiculo;
  const QString modelo;
  const QDateTime dataEvento;

  const QByteArray fileContent;
  QStandardItemModel model;

  // methods
private:
  auto montarArvore() -> void;
  auto readChild(const QDomElement &element, QStandardItem *elementItem) -> void;
};

#endif // XML_H
