#include "sql.h"

#include "application.h"
#include "sqlquery.h"

#include <QSqlError>
#include <QStringList>

void Sql::updateVendaStatus(const QStringList &idVendas) { updateVendaStatus(idVendas.join(", ")); }

void Sql::updateVendaStatus(const QString &idVendas) {
  QStringList list = idVendas.split(", ", Qt::SkipEmptyParts);
  list.removeDuplicates();

  for (auto const &idVenda : list) {
    if (SqlQuery query; not query.exec("CALL update_venda_status('" + idVenda + "')")) { throw RuntimeException("Erro atualizando status: " + query.lastError().text()); }
  }
}

// clang-format off

QString Sql::view_entrega_pendente(const QString &filtroBusca, const QString &filtroCheck, const QString &filtroStatus) {
  return "SELECT "
         "`v`.`data` AS `data`,"
         "CAST(`v`.`data` + INTERVAL `v`.`prazoEntrega` DAY AS DATE) AS `prazoEntrega`,"
         "CAST(`v`.`data` + INTERVAL `v`.`novoPrazoEntrega` DAY AS DATE) AS `novoPrazoEntrega`,"
         "CAST(MIN(`vp2`.`dataRealReceb`) AS DATE) AS `dataRealReceb`,"
         "`v`.`statusFinanceiro` AS `statusFinanceiro`,"
         "`vp2`.`idVenda` AS `idVenda`,"
         "SUM(`vp2`.`status` = 'ESTOQUE') AS `Estoque`,"
         "SUM(`vp2`.`status` IN ('ENTREGUE' , 'EM ENTREGA', 'ENTREGA AGEND.')) AS `Agend/Entregue`,"
         "SUM(`vp2`.`status` NOT IN ('ESTOQUE' , 'ENTREGUE', 'EM ENTREGA', 'ENTREGA AGEND.', 'DEVOLVIDO', 'QUEBRADO')) AS `Outros`,"
         "`che`.`bairro` AS `Bairro`,"
         "`che`.`logradouro` AS `Logradouro`,"
         "`che`.`numero` AS `Número`,"
         "`che`.`cidade` AS `Cidade` "
         "FROM "
         "`venda_has_produto2` `vp2` "
         "LEFT JOIN `venda` `v` ON `vp2`.`idVenda` = `v`.`idVenda` "
         "LEFT JOIN `cliente_has_endereco` `che` ON `v`.`idEnderecoEntrega` = `che`.`idEndereco` "
         "WHERE "
         "(`vp2`.`idVenda` NOT LIKE '%D') "
         "AND (`v`.`status` NOT IN ('ENTREGUE' , 'CANCELADO', 'DEVOLVIDO')) "
         "AND (`v`.`representacao` = FALSE) "
         "AND (`v`.`devolucao` = FALSE) " +
         filtroBusca +
         filtroStatus +
         " GROUP BY `vp2`.`idVenda` " +
         filtroCheck;
}

QString Sql::view_agendar_entrega(const QString &idVenda, const QString &status) {
  QStringList filtros;
  if (not idVenda.isEmpty()) { filtros << "vp2.idVenda = '" + idVenda + "'"; }
  if (not status.isEmpty()) { filtros << status; }
  QString filtro;
  if (not filtros.isEmpty()) { filtro = "WHERE " + filtros.join(" AND "); }

  return "SELECT "
         "`vp2`.`idVendaProduto2` AS `idVendaProduto2`,"
         "`vp2`.`idProduto` AS `idProduto`,"
         "`vp2`.`dataPrevEnt` AS `dataPrevEnt`,"
         "`vp2`.`dataRealEnt` AS `dataRealEnt`,"
         "`vp2`.`status` AS `status`,"
         "`vp2`.`fornecedor` AS `fornecedor`,"
         "`vp2`.`idVenda` AS `idVenda`,"
         "`n`.`numeroNFe` AS `idNFeSaida`,"
         "`n2`.`numeroNFe` AS `idNFeFutura`,"
         "`vp2`.`produto` AS `produto`,"
         "GROUP_CONCAT(DISTINCT `e`.`idEstoque` SEPARATOR ', ') AS `idEstoque`,"
         "GROUP_CONCAT(DISTINCT `e`.`lote` SEPARATOR ', ') AS `lote`,"
         "GROUP_CONCAT(DISTINCT `e`.`local` SEPARATOR ', ') AS `local`,"
         "GROUP_CONCAT(DISTINCT `e`.`bloco` SEPARATOR ', ') AS `bloco`,"
         "`vp2`.`caixas` AS `caixas`,"
         "`vp2`.`quant` AS `quant`,"
         "`vp2`.`un` AS `un`,"
         "`vp2`.`quantCaixa` AS `quantCaixa`,"
         "`vp2`.`codComercial` AS `codComercial`,"
         "`vp2`.`formComercial` AS `formComercial`,"
         "GROUP_CONCAT(`ehc`.`idConsumo` SEPARATOR ',') AS `idConsumo` "
         "FROM `venda_has_produto2` `vp2` "
         "LEFT JOIN `estoque_has_consumo` `ehc` ON `vp2`.`idVendaProduto2` = `ehc`.`idVendaProduto2` "
         "LEFT JOIN `estoque` `e` ON `ehc`.`idEstoque` = `e`.`idEstoque` "
         "LEFT JOIN `nfe` `n` ON `vp2`.`idNFeSaida` = `n`.`idNFe` "
         "LEFT JOIN `nfe` `n2` ON `vp2`.`idNFeFutura` = `n2`.`idNFe` " +
         filtro +
         " GROUP BY `vp2`.`idVendaProduto2`";
}

QString Sql::view_a_receber_vencidos() {
    return "SELECT "
           "v.*, "
           "@running_total:=@running_total + v.Total AS Acumulado "
           "FROM "
           "(SELECT "
           "`cr`.`dataPagamento` AS `Data Pagamento`, "
           "`cr`.`status` AS `Status`, "
           "SUM(IF((((`cr`.`tipo` LIKE '%CARTÃO%')"
           "      OR (`cr`.`tipo` LIKE '%CRÉDITO%')"
           "      OR (`cr`.`tipo` LIKE '%DÉBITO%'))"
           "      AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Cartão`, "
           "SUM(IF(((`cr`.`tipo` LIKE '%CHEQUE%')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Cheque`, "
           "SUM(IF(((`cr`.`tipo` LIKE '%BOLETO%')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Boleto`, "
           "SUM(IF(((NOT ((`cr`.`tipo` LIKE '%CARTÃO%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%CRÉDITO%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%DÉBITO%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%CHEQUE%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%BOLETO%')))"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Outros`, "
           "SUM(`cr`.`valor`) AS `Total` "
           "FROM "
           "`conta_a_receber_has_pagamento` `cr` "
           "WHERE "
           "`cr`.`desativado` = FALSE "
           "GROUP BY `cr`.`dataPagamento` , `cr`.`representacao` , `cr`.`status` "
           "HAVING `cr`.`dataPagamento` < CURDATE() "
           "AND `cr`.`representacao` = 0 "
           "AND cr.status IN ('PENDENTE' , 'CONFERIDO')) v "
           "JOIN "
           "(SELECT @running_total:=0) r";
}

QString Sql::view_a_receber_vencer() {
    return "SELECT "
           "v.*, "
           "@running_total:=@running_total + v.Total AS Acumulado "
           "FROM "
           "(SELECT "
           "`cr`.`dataPagamento` AS `Data Pagamento`, "
           "`cr`.`status` AS `Status`, "
           "SUM(IF((((`cr`.`tipo` LIKE '%CARTÃO%')"
           "      OR (`cr`.`tipo` LIKE '%CRÉDITO%')"
           "      OR (`cr`.`tipo` LIKE '%DÉBITO%'))"
           "      AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Cartão`, "
           "SUM(IF(((`cr`.`tipo` LIKE '%CHEQUE%')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Cheque`, "
           "SUM(IF(((`cr`.`tipo` LIKE '%BOLETO%')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Boleto`, "
           "SUM(IF(((NOT ((`cr`.`tipo` LIKE '%CARTÃO%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%CRÉDITO%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%DÉBITO%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%CHEQUE%')))"
           "    AND (NOT ((`cr`.`tipo` LIKE '%BOLETO%')))"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `Outros`, "
           "SUM(`cr`.`valor`) AS `Total` "
           "FROM "
           "`conta_a_receber_has_pagamento` `cr` "
           "WHERE "
           "`cr`.`desativado` = FALSE "
           "GROUP BY `cr`.`dataPagamento` , `cr`.`representacao` , `cr`.`status` "
           "HAVING `cr`.`dataPagamento` >= CURDATE() "
           "AND `cr`.`representacao` = 0 "
           "AND cr.status IN ('PENDENTE' , 'CONFERIDO')) v "
           "JOIN "
           "(SELECT @running_total:=0) r";
}

QString Sql::view_a_pagar_vencidos() {
    return "SELECT "
           "v.*, "
           "@running_total:=@running_total + v.Total AS Acumulado "
           "FROM "
           "(SELECT "
           "`cr`.`dataPagamento` AS `Data Pagamento`, "
           "SUM(IF(((`cr`.`status` = 'PENDENTE')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `PENDENTE`, "
           "SUM(IF(((`cr`.`status` = 'CONFERIDO')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `CONFERIDO`, "
           "SUM(IF(((`cr`.`status` = 'AGENDADO')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `AGENDADO`, "
           "SUM(IF(((`cr`.`status` IN ('PENDENTE GARE' , 'LIBERADO GARE', 'GERADO GARE'))"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `GARE`, "
           "SUM(`cr`.`valor`) AS `Total` "
           "FROM "
           "`conta_a_pagar_has_pagamento` `cr` "
           "WHERE "
           "`cr`.`desativado` = FALSE "
           "AND `cr`.`dataPagamento` < CURDATE() "
           "AND `cr`.`status` IN ('PENDENTE' , 'CONFERIDO', 'AGENDADO', 'PENDENTE GARE', 'LIBERADO GARE', 'GERADO GARE') "
           "GROUP BY `cr`.`dataPagamento`) v "
           "JOIN "
           "(SELECT @running_total:=0) r";
}

QString Sql::view_a_pagar_vencer() {
    return "SELECT "
           "v.*, "
           "@running_total:=@running_total + v.Total AS Acumulado "
           "FROM "
           "(SELECT "
           "`cr`.`dataPagamento` AS `Data Pagamento`, "
           "SUM(IF(((`cr`.`status` = 'PENDENTE')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `PENDENTE`, "
           "SUM(IF(((`cr`.`status` = 'CONFERIDO')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `CONFERIDO`, "
           "SUM(IF(((`cr`.`status` = 'AGENDADO')"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `AGENDADO`, "
           "SUM(IF(((`cr`.`status` IN ('PENDENTE GARE' , 'LIBERADO GARE', 'GERADO GARE'))"
           "    AND (`cr`.`desativado` = FALSE)), "
           " `cr`.`valor`, "
           " 0)) AS `GARE`, "
           "SUM(`cr`.`valor`) AS `Total` "
           "FROM "
           "`conta_a_pagar_has_pagamento` `cr` "
           "WHERE "
           "`cr`.`desativado` = FALSE "
           "AND `cr`.`dataPagamento` >= CURDATE() "
           "AND `cr`.`status` IN ('PENDENTE' , 'CONFERIDO', 'AGENDADO', 'PENDENTE GARE', 'LIBERADO GARE', 'GERADO GARE') "
           "GROUP BY `cr`.`dataPagamento`) v "
           "JOIN "
           "(SELECT @running_total:=0) r";
}

// clang-format on

// TODO: como a devolucao vai entrar no fluxo de logistica o status dos produtos não vão mais ser fixos e devem ser alterados nessas querys tambem
// FIXME: recebimento de estoque altera os consumos que por sua vez altera venda_has_produto mas depende do pedido_fornecedor_has_produto ter vp.idVenda preenchido para esta função funcionar
// TODO: centralizar sql das views em forma de código aqui usando parametros para preencher WHERE? (mais rápido usar WHERE dentro da query do que fora da view)
