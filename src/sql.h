#pragma once

#include <QString>

class Sql {
public:
  Sql() = delete;
  static auto updateVendaStatus(const QString &idVendas) -> bool;
  static auto updateVendaStatus(const QStringList &idVendas) -> bool;
  static auto view_agendar_entrega(const QString &idVenda = QString(), const QString &status = QString()) -> QString;
  static auto view_entrega_pendente(const QString &filtroBusca = QString(), const QString &filtroCheck = QString(), const QString &filtroStatus = QString()) -> QString;
};
