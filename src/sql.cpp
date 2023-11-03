#include "sql.h"

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

QString Sql::view_entrega_pendente(const QString &filtroBusca, const QString &filtroCheck, const QString &filtroStatus, const QString& filtroAtelier, const QString& filtroServico) {
  return " SELECT "
         "     SUM(IF(vp2.status NOT IN ('ENTREGUE' , 'CANCELADO', 'DEVOLVIDO'), CAST(p.kgcx * vp2.caixas AS signed), 0)) AS kg,"
         "     `che`.`cep` AS `CEP`,"
         "     `che`.`bairro` AS `Bairro`,"
         "     `che`.`logradouro` AS `Logradouro`,"
         "     `che`.`numero` AS `Número`,"
         "     `che`.`cidade` AS `Cidade`,"
         "     `v`.`data` AS `data`,"
         "     CAST(`v`.`data` + INTERVAL `v`.`prazoEntrega` DAY AS DATE) AS `prazoEntrega`,"
         "     CAST(`v`.`data` + INTERVAL `v`.`novoPrazoEntrega` DAY AS DATE) AS `novoPrazoEntrega`,"
         "     CAST(MIN(`vp2`.`dataRealReceb`) AS DATE) AS `dataRealReceb`,"
         "     `v`.`statusFinanceiro` AS `statusFinanceiro`,"
         "     `vp2`.`idVenda` AS `idVenda`,"
         "     SUM(`vp2`.`status` = 'ESTOQUE') AS `Estoque`,"
         "     SUM(`vp2`.`status` IN ('ENTREGUE' , 'EM ENTREGA', 'ENTREGA AGEND.')) AS `Agend/Entregue`,"
         "     SUM(`vp2`.`status` NOT IN ('ESTOQUE' , 'ENTREGUE', 'EM ENTREGA', 'ENTREGA AGEND.', 'DEVOLVIDO', 'QUEBRADO')) AS `Outros`,"
         "     CONCAT(lat, ';', lng, ';', REPLACE(v.idVenda,'&','&amp'), ';', REPLACE(che.logradouro, ' ', '%20'), ',%20', REPLACE(che.numero, ' ', '%20'), '%20-%20', REPLACE(che.bairro, ' ', '%20'), ',%20', REPLACE(che.cidade, ' ', '%20')) AS mapa"
         " FROM "
         "     `venda_has_produto2` `vp2` "
         " LEFT JOIN "
         "     `venda` `v` ON `vp2`.`idVenda` = `v`.`idVenda` "
         " LEFT JOIN "
         "     `cliente_has_endereco` `che` ON `v`.`idEnderecoEntrega` = `che`.`idEndereco` "
         " LEFT JOIN "
         "     `produto` `p` ON `vp2`.`idProduto` = `p`.`idProduto` "
         " WHERE "
         "     (`vp2`.`idVenda` NOT LIKE '%D' AND `vp2`.`quant` != 0) "
         "     AND (`v`.`status` NOT IN ('ENTREGUE' , 'CANCELADO', 'DEVOLVIDO')) "
         "     AND (`v`.`representacao` = FALSE) "
         "     AND (`v`.`devolucao` = FALSE) " +
         filtroBusca +
         filtroStatus +
         filtroAtelier +
         filtroServico +
         " GROUP BY "
         "     `vp2`.`idVenda` " +
         filtroCheck;
}

QString Sql::view_agendar_entrega(const QString &idVenda, const QString &status) {
  QStringList filtros;
  if (not idVenda.isEmpty()) { filtros << "vp2.idVenda = '" + idVenda + "'"; }
  if (not status.isEmpty()) { filtros << status; }
  QString filtro;
  if (not filtros.isEmpty()) { filtro = "WHERE " + filtros.join(" AND "); }

  return " SELECT "
         "     `vp2`.`idVendaProduto2` AS `idVendaProduto2`,"
         "     `vp2`.`idProduto` AS `idProduto`,"
         "     `vp2`.`dataPrevEnt` AS `dataPrevEnt`,"
         "     `vp2`.`dataRealEnt` AS `dataRealEnt`,"
         "     `vp2`.`status` AS `status`,"
         "     `vp2`.`fornecedor` AS `fornecedor`,"
         "     `vp2`.`idVenda` AS `idVenda`,"
         "     `n`.`idNFe` AS `idNFeSaida`,"
         "     `n2`.`idNFe` AS `idNFeFutura`,"
         "     `n`.`numeroNFe` AS `nfeSaida`,"
         "     `n2`.`numeroNFe` AS `nfeFutura`,"
         "     `vp2`.`produto` AS `produto`,"
         "     GROUP_CONCAT(DISTINCT `e`.`idEstoque` SEPARATOR ', ') AS `idEstoque`,"
         "     GROUP_CONCAT(DISTINCT `e`.`lote` SEPARATOR ', ') AS `lote`,"
         "     GROUP_CONCAT(DISTINCT `e`.`local` SEPARATOR ', ') AS `local`,"
         "     GROUP_CONCAT(DISTINCT `e`.`bloco` SEPARATOR ', ') AS `bloco`,"
         "     `vp2`.`caixas` AS `caixas`,"
         "     `vp2`.`quant` AS `quant`,"
         "     `vp2`.`un` AS `un`,"
         "     `vp2`.`quantCaixa` AS `quantCaixa`,"
         "     `vp2`.`codComercial` AS `codComercial`,"
         "     `vp2`.`formComercial` AS `formComercial`,"
         "     GROUP_CONCAT(`ehc`.`idConsumo` SEPARATOR ',') AS `idConsumo` "
         " FROM "
         "     `venda_has_produto2` `vp2` "
         " LEFT JOIN "
         "     `estoque_has_consumo` `ehc` ON `vp2`.`idVendaProduto2` = `ehc`.`idVendaProduto2` "
         " LEFT JOIN "
         "     `estoque` `e` ON `ehc`.`idEstoque` = `e`.`idEstoque` "
         " LEFT JOIN "
         "     `nfe` `n` ON `vp2`.`idNFeSaida` = `n`.`idNFe` "
         " LEFT JOIN "
         "     `nfe` `n2` ON `vp2`.`idNFeFutura` = `n2`.`idNFe` " +
         filtro +
         " GROUP BY"
         "     `vp2`.`idVendaProduto2`";
}

QString Sql::view_a_receber_vencidos() {
    return " SELECT "
           "     `cr`.`dataPagamento` AS `Data Pagamento`, "
           "     `cr`.`status` AS `Status`, "
           "     SUM("
           "         IF((((`cr`.`tipo` LIKE '%CARTÃO%') OR (`cr`.`tipo` LIKE '%CRÉDITO%') OR (`cr`.`tipo` LIKE '%DÉBITO%'))"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Cartão`, "
           "     SUM("
           "         IF(((`cr`.`tipo` LIKE '%CHEQUE%')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Cheque`, "
           "     SUM("
           "         IF(((`cr`.`tipo` LIKE '%BOLETO%')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Boleto`, "
           "     SUM("
           "     IF(((NOT ((`cr`.`tipo` LIKE '%CARTÃO%')))"
           "     AND (NOT ((`cr`.`tipo` LIKE '%CRÉDITO%')))"
           "     AND (NOT ((`cr`.`tipo` LIKE '%DÉBITO%')))"
           "     AND (NOT ((`cr`.`tipo` LIKE '%CHEQUE%')))"
           "     AND (NOT ((`cr`.`tipo` LIKE '%BOLETO%')))"
           "     AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Outros`, "
           "     SUM(`cr`.`valor`) AS `Total`, "
           "     SUM(SUM(`cr`.`valor`)) OVER (ORDER BY dataPagamento, representacao, status) AS `Acumulado` "
           " FROM "
           "     `conta_a_receber_has_pagamento` `cr` "
           "WHERE "
           "     `cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO')" // TODO: como estou removendo os cancelados aqui precisa mesmo filtrar no SUM()??
           " GROUP BY "
           "     `cr`.`dataPagamento` , `cr`.`representacao` , `cr`.`status` "
           " HAVING "
           "     `cr`.`dataPagamento` < CURDATE() "
           "     AND `cr`.`representacao` = 0 "
           "     AND cr.status IN ('PENDENTE' , 'CONFERIDO')"; // TODO: precisa colocar status aqui??
}

QString Sql::view_a_receber_vencer() {
    return " SELECT "
           "     `cr`.`dataPagamento` AS `Data Pagamento`, "
           "     `cr`.`status` AS `Status`, "
           "     SUM("
           "         IF((((`cr`.`tipo` LIKE '%CARTÃO%')"
           "         OR (`cr`.`tipo` LIKE '%CRÉDITO%')"
           "         OR (`cr`.`tipo` LIKE '%DÉBITO%'))"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Cartão`, "
           "     SUM("
           "         IF(((`cr`.`tipo` LIKE '%CHEQUE%')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Cheque`, "
           "     SUM("
           "         IF(((`cr`.`tipo` LIKE '%BOLETO%')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Boleto`, "
           "     SUM("
           "         IF(((NOT ((`cr`.`tipo` LIKE '%CARTÃO%')))"
           "         AND (NOT ((`cr`.`tipo` LIKE '%CRÉDITO%')))"
           "         AND (NOT ((`cr`.`tipo` LIKE '%DÉBITO%')))"
           "         AND (NOT ((`cr`.`tipo` LIKE '%CHEQUE%')))"
           "         AND (NOT ((`cr`.`tipo` LIKE '%BOLETO%')))"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `Outros`, "
           "     SUM(`cr`.`valor`) AS `Total`, "
           "     SUM(SUM(`cr`.`valor`)) OVER (ORDER BY dataPagamento, representacao, status) AS `Acumulado` "
           " FROM "
           "     `conta_a_receber_has_pagamento` `cr` "
           " WHERE "
           "     `cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO')"
           " GROUP BY "
           "     `cr`.`dataPagamento` , `cr`.`representacao` , `cr`.`status` "
           " HAVING "
           "     `cr`.`dataPagamento` >= CURDATE() "
           "     AND `cr`.`representacao` = 0 "
           "     AND cr.status IN ('PENDENTE' , 'CONFERIDO')";
}

QString Sql::view_a_pagar_vencidos() {
    return " SELECT "
           "     `cr`.`dataPagamento` AS `Data Pagamento`, "
           "     SUM("
           "         IF(((`cr`.`status` = 'PENDENTE')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `PENDENTE`, "
           "     SUM("
           "         IF(((`cr`.`status` = 'CONFERIDO')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `CONFERIDO`, "
           "     SUM("
           "         IF(((`cr`.`status` = 'AGENDADO')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `AGENDADO`, "
           "     SUM("
           "         IF(((`cr`.`status` IN ('PENDENTE GARE' , 'LIBERADO GARE', 'GERADO GARE'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `GARE`, "
           "     SUM(`cr`.`valor`) AS `Total`, "
           "     SUM(SUM(`cr`.`valor`)) OVER (ORDER BY dataPagamento) AS `Acumulado` "
           " FROM "
           "     `conta_a_pagar_has_pagamento` `cr` "
           " WHERE "
           "     `cr`.`dataPagamento` < CURDATE() "
           "     AND `cr`.`status` IN ('PENDENTE' , 'CONFERIDO', 'AGENDADO', 'PENDENTE GARE', 'LIBERADO GARE', 'GERADO GARE') "
           " GROUP BY "
           "     `cr`.`dataPagamento`";
}

QString Sql::view_a_pagar_vencer() {
    return " SELECT "
           "     `cr`.`dataPagamento` AS `Data Pagamento`, "
           "     SUM("
           "         IF(((`cr`.`status` = 'PENDENTE')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `PENDENTE`, "
           "     SUM("
           "         IF(((`cr`.`status` = 'CONFERIDO')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `CONFERIDO`, "
           "     SUM("
           "         IF(((`cr`.`status` = 'AGENDADO')"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `AGENDADO`, "
           "     SUM("
           "         IF(((`cr`.`status` IN ('PENDENTE GARE' , 'LIBERADO GARE', 'GERADO GARE'))"
           "         AND (`cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO'))), "
           "     `cr`.`valor`, 0)) "
           "         AS `GARE`, "
           "     SUM(`cr`.`valor`) AS `Total`, "
           "     SUM(SUM(`cr`.`valor`)) OVER (ORDER BY dataPagamento) AS `Acumulado`"
           " FROM "
           "     `conta_a_pagar_has_pagamento` `cr` "
           " WHERE "
           "     `cr`.`status` NOT IN ('CANCELADO', 'SUBSTITUIDO')"
           "     AND `cr`.`dataPagamento` >= CURDATE() "
           "     AND `cr`.`status` IN ('PENDENTE' , 'CONFERIDO', 'AGENDADO', 'PENDENTE GARE', 'LIBERADO GARE', 'GERADO GARE') "
           " GROUP BY "
           "     `cr`.`dataPagamento`";
}

QString Sql::view_relatorio_loja(const QString& mes, const QString &idUsuario, const QString &idUsuarioConsultor, const QString &loja){
  QStringList filtros;
  if (not mes.isEmpty()) { filtros << mes; }
  if (not idUsuario.isEmpty()) { filtros << "idUsuario = " + idUsuario; }
  if (not idUsuarioConsultor.isEmpty()) { filtros << "idUsuarioConsultor = " + idUsuarioConsultor; }
  if (not loja.isEmpty()) { filtros << "v.loja = '" + loja + "'"; }
  QString filtro;
  if (not filtros.isEmpty()) { filtro = "WHERE " + filtros.join(" AND "); }

  return " SELECT "
         "     `v`.`Loja` AS `Loja`,"
         "     SUM(`v`.`Faturamento`) AS `Faturamento`,"
         "     SUM(`v`.`Comissão`) AS `Comissão`,"
         "     ((SUM(`v`.`Comissão`) / SUM(`v`.`Faturamento`)) * 100) AS `porcentagem`,"
         "     `v`.`Mês` AS `Mês`,"
         "     (GROUP_CONCAT(DISTINCT `r`.`custoReposicao` SEPARATOR ',') + 0.0) AS `Reposição` "
         " FROM "
         "     `view_relatorio` `v` "
         " LEFT JOIN "
         "     view_relatorio_reposicao r ON `r`.`loja` = `v`.`Loja` AND r.data = v.Mês " +
         filtro +
         " GROUP BY "
         "     `v`.`Loja`, Mês ";
}

QString Sql::view_relatorio_vendedor(const QString& mes, const QString &idUsuario, const QString &idUsuarioConsultor, const QString &loja){
  QStringList filtros;
  if (not mes.isEmpty()) { filtros << mes; }
  if (not idUsuario.isEmpty()) { filtros << "idUsuario = " + idUsuario; }
  if (not idUsuarioConsultor.isEmpty()) { filtros << "idUsuarioConsultor = " + idUsuarioConsultor; }
  if (not loja.isEmpty()) { filtros << "loja = '" + loja + "'"; }
  QString filtro;
  if (not filtros.isEmpty()) { filtro = "WHERE " + filtros.join(" AND "); }

  return " SELECT "
         "     `c`.`Loja` AS `Loja`,"
         "     GROUP_CONCAT(DISTINCT `c`.`Vendedor`) AS `Vendedor`,"
         "     `c`.`idUsuario` AS `idUsuario`,"
         "     GROUP_CONCAT(DISTINCT `c`.`idUsuarioConsultor` SEPARATOR ',') AS `idUsuarioConsultor`,"
         "     SUM(`c`.`Faturamento`) AS `Faturamento`,"
         "     SUM(`c`.`Comissão`) AS `Comissão`,"
         "     ((SUM(`c`.`Comissão`) / SUM(`c`.`Faturamento`)) * 100) AS `porcentagem`,"
         "     `c`.`Mês` AS `Mês` "
         " FROM "
         "     `comissao` `c` "
         + filtro +
         " GROUP BY "
         "     `idUsuario` , `Loja` , `Mês` ";
}

QString Sql::view_estoque_contabil(const QString &match, const QString &data) {
  // e.created é usado no lugar de pf2.dataRealReceb porque tem vários estoques sem vínculo compra

  return " WITH ehc AS"
         " ("
         "     SELECT "
         "         ehc.idEstoque,"
         "         SUM(ehc.quant) AS contabil,"
         "         SUM(IF(vp.status != 'DEVOLVIDO ESTOQUE', vp.quant, 0)) AS consumoVenda,"
         "         GROUP_CONCAT(DISTINCT vp.idVenda) AS idVenda"
         "     FROM"
         "         estoque_has_consumo ehc"
         "     LEFT JOIN "
         "         venda_has_produto2 vp ON ehc.idVendaProduto2 = vp.idVendaProduto2"
         "     WHERE"
         "         vp.dataRealEnt <= '" + data + "' AND ehc.status != 'CANCELADO'"
         "     GROUP BY ehc.idEstoque"
         " )"
         " SELECT "
         "     n.cnpjDest,"
         "     e.status,"
         "     e.idEstoque,"
         "     p.fornecedor,"
         "     e.descricao,"
         "     (e.quant + e.ajuste + COALESCE(ehc.contabil, 0)) / p.quantCaixa AS caixasContabil,"
         "     e.quant + e.ajuste + COALESCE(ehc.contabil, 0) AS contabil,"
         "     e.restante / p.quantCaixa AS caixas,"
         "     e.restante,"
         "     e.un AS unEst,"
         "     p.un AS unProd,"
         "     e.lote,"
         "     e.local,"
         "     g.label,"
         "     e.codComercial,"
         "     p.formComercial,"
         "     e.ncm,"
         "     e.cstICMS,"
         "     e.pICMS,"
         "     e.cstIPI,"
         "     e.cstPIS,"
         "     e.cstCOFINS,"
         "     n.numeroNFe AS nfe,"
         "     e.valorUnid AS custoUnit,"
         "     p.precoVenda AS precoVendaUnit,"
         "     e.valorUnid * (e.quant + e.ajuste + COALESCE(ehc.contabil, 0)) AS custo,"
         "     p.precoVenda * (e.quant + e.ajuste + COALESCE(ehc.contabil, 0)) AS precoVenda,"
         "     ehc.idVenda,"
         "     ANY_VALUE(pf2.dataPrevColeta) AS dataPrevColeta,"
         "     ANY_VALUE(pf2.dataRealColeta) AS dataRealColeta,"
         "     ANY_VALUE(pf2.dataPrevReceb) AS dataPrevReceb,"
         "     ANY_VALUE(pf2.dataRealReceb) AS dataRealReceb"
         " FROM"
         "     estoque e"
         " LEFT JOIN"
         "	   ehc ON e.idEstoque = ehc.idEstoque"
         " LEFT JOIN"
         "     estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque"
         " LEFT JOIN"
         "     pedido_fornecedor_has_produto2 pf2 ON pf2.idPedido2 = ehc2.idPedido2"
         " LEFT JOIN"
         "     nfe n ON e.idNFe = n.idNFe"
         " LEFT JOIN"
         "     produto p ON e.idProduto = p.idProduto"
         " LEFT JOIN"
         "     galpao g ON e.idBloco = g.idBloco"
         " WHERE"
         "     e.status = 'ESTOQUE' AND DATE(e.created) <= '" + data + "' "
         + match +
         " GROUP BY "
         "     e.idEstoque"
         " HAVING "
         "     contabil > 0";
}

QString Sql::queryEstoque(const QString &match, const QString &having) {
  return " SELECT "
         "     n.cnpjDest,"
         "     e.status,"
         "     e.idEstoque,"
         "     p.fornecedor,"
         "     e.descricao,"
         "     e.restante / p.quantCaixa AS caixas,"
         "     e.restante AS restante,"
         "     e.un AS unEst,"
         "     e.lote,"
         "     e.local,"
         "     g.label,"
         "     e.codComercial,"
         "     p.formComercial,"
         "     n.numeroNFe AS nfe,"
         "     ANY_VALUE(pf2.dataPrevColeta) AS dataPrevColeta,"
         "     ANY_VALUE(pf2.dataRealColeta) AS dataRealColeta,"
         "     ANY_VALUE(pf2.dataPrevReceb) AS dataPrevReceb,"
         "     ANY_VALUE(pf2.dataRealReceb) AS dataRealReceb"
         " FROM"
         "     estoque e"
         "         LEFT JOIN"
         "     estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque"
         "         LEFT JOIN"
         "     pedido_fornecedor_has_produto2 pf2 ON pf2.idPedido2 = ehc2.idPedido2"
         "         LEFT JOIN"
         "     nfe n ON e.idNFe = n.idNFe"
         "         LEFT JOIN"
         "     produto p ON e.idProduto = p.idProduto"
         "         LEFT JOIN"
         "     galpao g ON e.idBloco = g.idBloco"
         " WHERE"
         "     e.status NOT IN ('CANCELADO', 'IGNORAR') "
         + match +
         " GROUP BY "
         "     e.idEstoque"
         " HAVING "
         + having;
}

QString Sql::queryExportarNCM() {
  return " SELECT "
         "     e.fornecedor,"
         "     EXTRACTVALUE(n.xml, '/nfeProc/NFe/infNFe/emit/CNPJ') AS cnpj,"
         "     EXTRACTVALUE(n.xml, '/nfeProc/NFe/infNFe/emit/enderEmit/UF') AS uf,"
         "     e.descricao,"
         "     e.ncm,"
         "     LPAD(e.cstICMS, 3, '0') AS cst"
         " FROM"
         "     estoque e"
         "         LEFT JOIN"
         "     nfe n ON e.idNFe = n.idNFe"
         " WHERE"
         "     e.contabil > 0";
}

QString Sql::contasPagar(const QString &filtros, const QString &busca) {
  QString where;
  if (not filtros.isEmpty()) { where = "WHERE " + filtros; }

  return " SELECT * FROM "
         "     ("
         "      SELECT "
         "          `cp`.`idPagamento` AS `idPagamento`, "
         "          `cp`.`idLoja` AS `idLoja`, "
         "          `cp`.`contraParte` AS `contraparte`, "
         "          `cp`.`dataEmissao` AS `dataEmissao`, "
         "          `cp`.`dataPagamento` AS `dataPagamento`, "
         "          `cp`.`dataRealizado` AS `dataRealizado`, "
         "          `cp`.`idVenda` AS `idVenda`, "
         "          GROUP_CONCAT(DISTINCT `pf2`.`ordemCompra` SEPARATOR ',') AS `ordemCompra`, "
         "          GROUP_CONCAT(DISTINCT `n`.`numeroNFe` SEPARATOR ', ') AS `numeroNFe`, "
         "          GROUP_CONCAT(DISTINCT `n`.`idNFe` SEPARATOR ', ') AS `idNFe`, "
         "          `cp`.`status` AS `status`, "
         "          `cp`.`valor` AS `valor`, "
         "          `cp`.`tipo` AS `tipo`, "
         "          `cp`.`parcela` AS `parcela`, "
         "          `cp`.`observacao` AS `observacao`, "
         "          `cp`.`grupo` AS `grupo`, "
         "          GROUP_CONCAT(DISTINCT `pf2`.`statusFinanceiro` SEPARATOR ',') AS `statusFinanceiro`, "
         "          GROUP_CONCAT(DISTINCT `pf2`.`idVenda` SEPARATOR ', ') AS `pf2_idVenda`, "
         "          GROUP_CONCAT(DISTINCT `pf2`.`codFornecedor` SEPARATOR ', ') AS `codFornecedor` "
         "      FROM "
         "          `conta_a_pagar_has_pagamento` `cp` "
         "      LEFT JOIN "
         "          `pedido_fornecedor_has_produto2` `pf2` ON `cp`.`idCompra` = `pf2`.`idCompra` "
         "      LEFT JOIN "
         "          `estoque_has_compra` `ehc` ON `ehc`.`idPedido2` = `pf2`.`idPedido2` "
         "      LEFT JOIN "
         "          `estoque` `e` ON `ehc`.`idEstoque` = `e`.`idEstoque` "
         "      LEFT JOIN "
         "          `nfe` `n` ON `n`.`idNFe` = `e`.`idNFe` "
         + where +
         "      GROUP BY "
         "          `cp`.`idPagamento`"
         "     ) x " +
         busca;
}

QString Sql::contasReceber(const QString &filtros) {
  QString where;
  if (not filtros.isEmpty()) { where = "WHERE " + filtros; }

  return " SELECT "
         "     `cr`.`idPagamento` AS `idPagamento`, "
         "     `cr`.`idLoja` AS `idLoja`, "
         "     `cr`.`representacao` AS `representacao`, "
         "     `cr`.`contraParte` AS `contraparte`, "
         "     `cr`.`dataEmissao` AS `dataEmissao`, "
         "     `cr`.`dataPagamento` AS `dataPagamento`, "
         "     `cr`.`dataRealizado` AS `dataRealizado`, "
         "     `cr`.`idVenda` AS `idVenda`, "
         "     GROUP_CONCAT(DISTINCT `pf2`.`ordemRepresentacao`) AS ordemRepresentacao, "
         "     `cr`.`status` AS `status`, "
         "     `cr`.`valor` AS `valor`, "
         "     `cr`.`tipo` AS `tipo`, "
         "     `cr`.`parcela` AS `parcela`, "
         "     `cr`.`observacao` AS `observacao`, "
         "     `v`.`statusFinanceiro` AS `statusFinanceiro` "
         " FROM "
         "     `conta_a_receber_has_pagamento` `cr` "
         " LEFT JOIN "
         "     `venda` `v` ON `cr`.`idVenda` = `v`.`idVenda` "
         " LEFT JOIN "
         "     pedido_fornecedor_has_produto2 pf2 ON v.idVenda = pf2.idVenda "
         + where +
         " GROUP BY "
         "     `cr`.`idPagamento`";
}

QString Sql::view_estoque(const QString &idEstoque) {
  return " SELECT "
         "     e.idEstoque, "
         "     e.idNFe, "
         "     e.recebidoPor, "
         "     e.status, "
         "     e.idProduto, "
         "     e.fornecedor, "
         "     e.descricao, "
         "     e.observacao, "
         "     e.lote, "
         "     e.idBloco, "
         "     e.local, "
         "     g.label, "
         "     e.quant, "
         "     e.quantUpd, "
         "     e.restante, "
         "     e.un, "
         "     e.caixas, "
         "     e.codBarras, "
         "     e.codComercial, "
         "     e.ncm, "
         "     e.cfop, "
         "     e.valorUnid, "
         "     p.quantCaixa, "
         "     e.codBarrasTrib, "
         "     e.unTrib, "
         "     e.quantTrib, "
         "     e.valorUnidTrib, "
         "     e.desconto, "
         "     e.compoeTotal, "
         "     e.numeroPedido, "
         "     e.itemPedido, "
         "     e.tipoICMS, "
         "     e.orig, "
         "     e.cstICMS, "
         "     e.modBC, "
         "     e.vBC, "
         "     e.pICMS, "
         "     e.vICMS, "
         "     e.modBCST, "
         "     e.pMVAST, "
         "     e.vBCST, "
         "     e.pICMSST, "
         "     e.vICMSST, "
         "     e.cEnq, "
         "     e.cstIPI, "
         "     e.cstPIS, "
         "     e.vBCPIS, "
         "     e.pPIS, "
         "     e.vPIS, "
         "     e.cstCOFINS, "
         "     e.vBCCOFINS, "
         "     e.pCOFINS, "
         "     e.vCOFINS "
         " FROM "
         "     estoque e "
         " LEFT JOIN "
         "     galpao g ON e.idBloco = g.idBloco "
         " LEFT JOIN "
         "     produto p ON e.idProduto = p.idProduto "
         " WHERE "
         "     e.idEstoque = " + idEstoque;
}

QString Sql::view_galpao(const QString &idBloco, const QString &filtroText) {
  // TODO: adicionar 'EM RECEBIMENTO'

  const QString filtroBloco = (not idBloco.isEmpty()) ? " AND g.idBloco = " + idBloco : "";
  const QString filtro = (not filtroText.isEmpty()) ? " AND (descricao LIKE '%"+filtroText+"%' OR codComercial LIKE '%" + filtroText + "%' OR numeroNFe LIKE '%" + filtroText + "%' OR "
                                                      "lote LIKE '%" + filtroText + "%' OR idVenda LIKE '%" + filtroText + "%')"
                                                    : "";

  return " SELECT "
         "     g.idBloco, "
         "     g.label, "
         "     v.idEstoque_idConsumo, "
         "     v.idEstoque, "
         "     v.tipo, "
         "     v.idVendaProduto2, "
         "     v.numeroNFe, "
         "     v.idNFe, "
         "     v.codComercial, "
         "     v.lote, "
         "     v.caixas, "
         "     v.idVenda, "
         "     v.descricao, "
         "     v.formComercial "
         " FROM "
         "     galpao g "
         " LEFT JOIN "
         "     view_galpao v ON g.idBloco = v.idBloco "
         " WHERE "
         "     idEstoque_idConsumo IS NOT NULL " +
         filtroBloco + filtro;
}

// clang-format on

// TODO: como a devolucao vai entrar no fluxo de logistica o status dos produtos não vão mais ser fixos e devem ser alterados nessas querys tambem
// FIXME: recebimento de estoque altera os consumos que por sua vez altera venda_has_produto mas depende do pedido_fornecedor_has_produto ter vp.idVenda preenchido para esta função funcionar
// TODO: centralizar sql das views em forma de código aqui usando parametros para preencher WHERE? (mais rápido usar WHERE dentro da query do que fora da view)

// TODO: substituir e.bloco por g.label
