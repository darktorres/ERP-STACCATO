#pragma once

#include <QString>

class Sql {

public:
  Sql() = delete;

  static auto updateVendaStatus(const QString &idVendas) -> void;
  static auto updateVendaStatus(const QStringList &idVendas) -> void;
  static auto view_a_pagar_vencer() -> QString;
  static auto view_a_pagar_vencidos() -> QString;
  static auto view_a_receber_vencer() -> QString;
  static auto view_a_receber_vencidos() -> QString;
  static auto view_agendar_entrega(const QString &idVenda = QString(), const QString &status = QString()) -> QString;
  static auto view_entrega_pendente(const QString &filtroBusca = QString(), const QString &filtroCheck = QString(), const QString &filtroStatus = QString()) -> QString;
  static auto view_relatorio_loja(const QString &mes = QString(), const QString &idUsuario = QString(), const QString &idUsuarioConsultor = QString(), const QString &loja = QString()) -> QString;
  static auto view_relatorio_vendedor(const QString &mes = QString(), const QString &idUsuario = QString(), const QString &idUsuarioConsultor = QString(), const QString &loja = QString()) -> QString;
};
