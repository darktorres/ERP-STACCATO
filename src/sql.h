#pragma once

#include "application.h"

#include <QString>

class Sql {

public:
  Sql() = delete;

  static auto contasPagar(const QString &filtros, const QString &busca) -> QString;
  static auto contasReceber(const QString &filtros) -> QString;
  static auto queryEstoque(const QString &match, const QString &having) -> QString;
  static auto queryExportarNCM() -> QString;
  static auto updateFornecedoresOrcamento(const QString idOrcamento) -> void;
  static auto updateFornecedoresVenda(const QString idVenda) -> void;
  static auto updateOrdemRepresentacaoVenda(const QString idVenda) -> void;
  static auto updateOrdemRepresentacaoVenda(const QStringList idVendas) -> void;
  static auto updateVendaStatus(const QString &idVendas) -> void;
  static auto updateVendaStatus(const QStringList &idVendas) -> void;
  static auto view_a_pagar_vencer() -> QString;
  static auto view_a_pagar_vencidos() -> QString;
  static auto view_a_receber_vencer() -> QString;
  static auto view_a_receber_vencidos() -> QString;
  static auto view_followup_venda_misto(const QString &idVenda) -> QString;
  static auto view_gare_vencer() -> QString;
  static auto view_gare_vencidos() -> QString;
  static auto view_agendar_entrega(const QString &idVenda = {}, const QString &status = {}) -> QString;
  static auto view_entrega_pendente(const QString &filtroBusca = {}, const QString &filtroCheck = {}, const QString &filtroStatus = {}, const QString &filtroAtelier = {},
                                    const QString &filtroServico = {}) -> QString;
  static auto view_estoque(const QString &idEstoque) -> QString;
  static auto view_estoque_contabil(const QString &match, const QString &data = qApp->serverDate().toString("yyyy-MM-dd")) -> QString;
  static auto view_galpao(const QString &idBloco, const QString &filtroText = {}) -> QString;
  static auto view_relatorio_loja(const QString &mes = {}, const QString &idUsuario = {}, const QString &idUsuarioConsultor = {}, const QString &loja = {}) -> QString;
  static auto view_relatorio_vendedor(const QString &mes = {}, const QString &idUsuario = {}, const QString &idUsuarioConsultor = {}, const QString &loja = {}) -> QString;
};
