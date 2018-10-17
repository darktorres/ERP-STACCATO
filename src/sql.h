#ifndef SQL_H
#define SQL_H

#include <QString>

class Sql {

public:
  auto updateVendaStatus(const QString &idVenda) -> bool;
};

#endif // SQL_H
