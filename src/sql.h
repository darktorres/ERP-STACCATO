#pragma once

#include <QString>

class Sql {
public:
  Sql() = delete;
  static auto updateVendaStatus(const QString &idVenda) -> bool;
  static auto updateVendaStatus(const QStringList &idVendas) -> bool;

private:
  static auto runQuerys(const QString &idVenda) -> bool;
};
