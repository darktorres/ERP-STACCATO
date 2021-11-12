#pragma once

#include "application.h"

#include <QString>

class Sql {

public:
  Sql() = delete;

  static auto contasPagar(const QString &filtros, const QString &busca) -> QString;
  static auto contasReceber(const QString &filtros) -> QString;
  static auto queryEstoque(const QString &match, const QString &having) -> QString;
  static auto updateVendaStatus(const QString &idVendas) -> void;
  static auto updateVendaStatus(const QStringList &idVendas) -> void;
  static auto view_a_pagar_vencer() -> QString;
  static auto view_a_pagar_vencidos() -> QString;
  static auto view_a_receber_vencer() -> QString;
  static auto view_a_receber_vencidos() -> QString;
  static auto view_agendar_entrega(const QString &idVenda = QString(), const QString &status = QString()) -> QString;
  static auto view_entrega_pendente(const QString &filtroBusca = QString(), const QString &filtroCheck = QString(), const QString &filtroStatus = QString(), const QString &filtroAtelier = QString(),
                                    const QString &filtroServico = QString()) -> QString;
  static auto view_estoque(const QString &idEstoque) -> QString;
  static auto view_estoque_contabil(const QString &match, const QString &data = qApp->serverDate().toString("yyyy-MM-dd")) -> QString;
  static auto view_relatorio_loja(const QString &mes = QString(), const QString &idUsuario = QString(), const QString &idUsuarioConsultor = QString(), const QString &loja = QString()) -> QString;
  static auto view_relatorio_vendedor(const QString &mes = QString(), const QString &idUsuario = QString(), const QString &idUsuarioConsultor = QString(), const QString &loja = QString()) -> QString;
};
