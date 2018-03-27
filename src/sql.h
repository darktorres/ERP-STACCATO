#ifndef SQL_H
#define SQL_H

#include <QObject>
#include <QString>

class Sql : public QObject {
  Q_OBJECT

public:
  Sql() = delete; // TODO: make this private so i can use signals
  static bool updateVendaStatus(const QString &idVenda);

signals:
  void errorSignal(const QString &error);
  void informationSignal(const QString &information);
  void transactionEnded();
  void transactionStarted();
};

#endif // SQL_H
