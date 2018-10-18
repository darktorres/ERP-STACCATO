#ifndef SQL_H
#define SQL_H

#include <QString>

class Sql {
public:
  Sql() = delete;
  static auto updateVendaStatus(const QString &idVenda) -> bool;
};

#endif // SQL_H
