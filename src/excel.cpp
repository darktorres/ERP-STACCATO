#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QSqlError>

#include "application.h"
#include "excel.h"
#include "usersession.h"

Excel::Excel(const QString &id, const Tipo tipo) : tipo(tipo), id(id) {}

void Excel::hideUnusedRows(QXlsx::Document &xlsx) {
  for (int row = queryProduto.size() + 12; row < 111; ++row) { xlsx.setRowHidden(row, true); }
}

bool Excel::gerarExcel(const int oc, const bool isRepresentacao, const QString &representacao) {
  // REFAC: dear god, divide this into smaller funcs

  const QString folder = tipo == Tipo::Orcamento ? "User/OrcamentosFolder" : "User/VendasFolder";

  const auto folderKey = UserSession::getSetting(folder);

  if (not folderKey) { return qApp->enqueueError(false, "Não há uma pasta definida para salvar PDF/Excel. Por favor escolha uma nas configurações do ERP!"); }

  const QString path = folderKey.value().toString();

  QDir dir(path);

  if (not dir.exists() and not dir.mkdir(path)) { return qApp->enqueueError(false, "Erro ao criar a pasta escolhida nas configurações!"); }

  const QString arquivoModelo = "modelo pedido.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) { return qApp->enqueueError(false, "Não encontrou o modelo do Excel!"); }

  if (not setQuerys()) { return false; }

  fileName = isRepresentacao ? path + "/" + representacao + ".xlsx"
                             : path + "/" + id + "-" + queryVendedor.value("nome").toString().split(" ").first() + "-" + queryCliente.value("nome_razao").toString().replace("/", "-") + ".xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) { return qApp->enqueueError(false, "Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

  file.close();

  QLocale locale;

  QXlsx::Document xlsx(arquivoModelo);

  xlsx.currentWorksheet()->setFitToPage(true);
  xlsx.currentWorksheet()->setFitToHeight(true);
  xlsx.currentWorksheet()->setOrientation(QXlsx::Worksheet::Orientation::Horizontal);

  const QString endLoja = queryLojaEnd.value("logradouro").toString() + ", " + queryLojaEnd.value("numero").toString() + " " + queryLojaEnd.value("complemento").toString() + " - " +
                          queryLojaEnd.value("bairro").toString() + "\n" + queryLojaEnd.value("cidade").toString() + " - " + queryLojaEnd.value("uf").toString() +
                          " - CEP: " + queryLojaEnd.value("cep").toString() + "\n" + queryLoja.value("tel").toString() + " - " + queryLoja.value("tel2").toString();

  const QString endEntrega = queryEndEnt.value("logradouro").toString().isEmpty()
                                 ? "Não há/Retira"
                                 : queryEndEnt.value("logradouro").toString() + " " + queryEndEnt.value("numero").toString() + " " + queryEndEnt.value("complemento").toString() + " - " +
                                       queryEndEnt.value("bairro").toString() + ", " + queryEndEnt.value("cidade").toString();

  const QString endFat = queryEndFat.value("logradouro").toString().isEmpty()
                             ? "Não há/Retira"
                             : queryEndFat.value("logradouro").toString() + " " + queryEndFat.value("numero").toString() + " " + queryEndFat.value("complemento").toString() + " - " +
                                   queryEndFat.value("bairro").toString() + ", " + queryEndFat.value("cidade").toString();

  xlsx.write("A5", endLoja);

  if (tipo == Tipo::Venda) {
    if (isRepresentacao) {
      xlsx.write("C2", "Pedido:");
      xlsx.write("D2", "OC " + QString::number(oc) + " " + id);
      QXlsx::Format format;
      format.setHorizontalAlignment(QXlsx::Format::AlignHCenter);
      format.setFontBold(true);
      format.setTopBorderStyle(QXlsx::Format::BorderMedium);
      format.setBottomBorderStyle(QXlsx::Format::BorderThin);
      xlsx.mergeCells(QXlsx::CellRange("D2:K2"), format);
    } else {
      xlsx.write("C2", "Pedido:");
      xlsx.write("D2", id);
      xlsx.write("H2", "Orçamento:");
      xlsx.write("I2", query.value("idOrcamento").toString());
    }
  }

  if (tipo == Tipo::Orcamento) { xlsx.write("D2", id); }

  xlsx.write("D3", queryCliente.value("nome_razao"));
  xlsx.write("D4", queryCliente.value("email"));
  xlsx.write("D5", endFat);
  xlsx.write("D6", endEntrega);
  xlsx.write("D7", queryProfissional.value("nome_razao"));
  xlsx.write("D8", queryVendedor.value("nome"));
  xlsx.write("F8", queryVendedor.value("email"));
  xlsx.write("M2", query.value("data").toDateTime().toString("dd/MM/yyyy hh:mm"));
  xlsx.write("M3", queryCliente.value(queryCliente.value("pfpj") == "PF" ? "cpf" : "cnpj"));
  xlsx.write("M4", queryCliente.value("tel"));
  xlsx.write("J4", queryCliente.value("telCel"));
  xlsx.write("M5", queryEndFat.value("cep"));
  xlsx.write("M6", queryEndEnt.value("cep"));
  xlsx.write("H7", queryProfissional.value("tel"));
  xlsx.write("K7", queryProfissional.value("email"));

  const double subLiq = query.value("subTotalLiq").toDouble();
  const double subBru = query.value("subTotalBru").toDouble();
  const double desconto = query.value("descontoPorc").toDouble();
  // TODO: 0no lugar de calcular valores, usar os do BD
  xlsx.write("N113", subLiq > subBru ? "R$ " + locale.toString(subLiq, 'f', 2) : "R$ " + locale.toString(subBru, 'f', 2) + " (R$ " + locale.toString(subLiq, 'f', 2) + ")"); // soma
  xlsx.write("N114", locale.toString(desconto, 'f', 2) + "%");                                                                                                               // desconto
  xlsx.write("N115", "R$ " + locale.toString(subLiq - (desconto / 100. * subLiq), 'f', 2));                                                                                  // total
  xlsx.write("N116", "R$ " + locale.toString(query.value("frete").toDouble(), 'f', 2));                                                                                      // frete
  xlsx.write("N117", "R$ " + locale.toString(query.value("total").toDouble(), 'f', 2));                                                                                      // total final
  xlsx.write("B113", query.value("prazoEntrega").toString() + " dias");

  const QString pgtQuery = "SELECT ANY_VALUE(tipo) AS tipo, COUNT(valor) AS parcelas, ANY_VALUE(valor) AS valor, ANY_VALUE(dataPagamento) AS dataPagamento, ANY_VALUE(observacao) AS observacao FROM "
                           "conta_a_receber_has_pagamento WHERE idVenda = '" +
                           id + "' AND tipo LIKE '%1%' AND tipo NOT IN ('%1. Comissão', '%1. Taxa Cartão') AND status NOT IN ('CANCELADO', 'SUBSTITUIDO')";

  for (int i = 1; i <= 5; ++i) {
    const QString current = QString::number(i);

    QSqlQuery queryPgt;

    if (not queryPgt.exec(pgtQuery.arg(current)) or not queryPgt.first()) { return qApp->enqueueError(false, "Erro buscando pagamento " + current + ": " + queryPgt.lastError().text()); }

    if (queryPgt.value("valor") == 0) { continue; }

    const QString pagEm = (queryPgt.value("parcelas") == 1 ? " - pag. em: " : " - 1° pag. em: ");
    const QString observacao = queryPgt.value("observacao").toString();

    const QString pgt = queryPgt.value("tipo").toString() + " - " + queryPgt.value("parcelas").toString() + "x de R$ " + locale.toString(queryPgt.value("valor").toDouble(), 'f', 2) + pagEm +
                        queryPgt.value("dataPagamento").toDate().toString("dd-MM-yyyy") + (observacao.isEmpty() ? "" : " - " + observacao);

    xlsx.write("B" + QString::number(113 + i), pgt);
  }

  xlsx.write("B119", query.value("observacao").toString().replace("\n", " "));

  // TODO: 5refator this to start at 12
  int row = 0;
  queryProduto.first();

  QSqlQuery queryUi; // TODO: put this query in 'setQuerys'?
  queryUi.prepare("SELECT ui FROM produto WHERE idProduto = :idProduto");

  do {
    queryUi.bindValue(":idProduto", queryProduto.value("idProduto"));

    if (not queryUi.exec() or not queryUi.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + queryUi.lastError().text()); }

    const QString loes = queryUi.value("ui").toString().contains("- L") ? " LOES" : "";

    xlsx.write("A" + QString::number(12 + row), queryProduto.value("fornecedor").toString() + loes);
    xlsx.write("B" + QString::number(12 + row), queryProduto.value("codComercial").toString());
    const QString formComercial = queryProduto.value("formComercial").toString();
    xlsx.write("C" + QString::number(12 + row), queryProduto.value("produto").toString() + (formComercial.isEmpty() ? "" : " (" + formComercial + ")") + (loes.isEmpty() ? "" : " -" + loes));
    xlsx.write("H" + QString::number(12 + row), queryProduto.value("obs").toString());

    const double prcUn = queryProduto.value("prcUnitario").toDouble();
    const double desc = prcUn * queryProduto.value("desconto").toDouble() / 100.;
    const double porc = queryProduto.value("desconto").toDouble();

    const QString preco = "R$ " + locale.toString(prcUn, 'f', 2);
    const QString precoDesc = desc > 0.01 ? " (-" + locale.toString(porc, 'f', 2) + "% R$ " + locale.toString(prcUn - desc, 'f', 2) + ")" : "";
    const QString precoDescNeg = "R$ " + locale.toString((porc / -100. + 1) * prcUn, 'f', 2);
    xlsx.write("K" + QString::number(12 + row), porc < 0 ? precoDescNeg : preco + precoDesc);
    xlsx.write("L" + QString::number(12 + row), queryProduto.value("quant").toDouble());
    xlsx.write("M" + QString::number(12 + row), queryProduto.value("un").toString());
    const QString total = "R$ " + locale.toString(queryProduto.value("parcial").toDouble(), 'f', 2);
    const QString totalDesc = desc > 0.01 ? " (R$ " + locale.toString(queryProduto.value("parcialDesc").toDouble(), 'f', 2) + ")" : "";
    const QString totalDescNeg = "R$ " + locale.toString(queryProduto.value("parcialDesc").toDouble(), 'f', 2);
    xlsx.write("N" + QString::number(12 + row), porc < 0 ? totalDescNeg : total + totalDesc);

    if (desc > 0.01) { xlsx.setColumnWidth(11, 28); }

    ++row;
  } while (queryProduto.next());

  hideUnusedRows(xlsx);

  if (not xlsx.saveAs(fileName)) { return qApp->enqueueError(false, "Ocorreu algum erro ao salvar o arquivo."); }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName);

  return true;
}

QString Excel::getFileName() const { return fileName; }

bool Excel::setQuerys() {
  if (tipo == Tipo::Orcamento) {
    query.prepare("SELECT idLoja, idUsuario, idProfissional, idEnderecoEntrega, idCliente, data, subTotalLiq, subTotalBru, descontoPorc, frete, total, prazoEntrega, observacao FROM orcamento WHERE "
                  "idOrcamento = :idOrcamento");
    query.bindValue(":idOrcamento", id);

    queryProduto.prepare(
        "SELECT idProduto, fornecedor, codComercial, formComercial, produto, obs, prcUnitario, desconto, quant, un, parcial, parcialDesc FROM orcamento_has_produto WHERE idOrcamento = :idOrcamento");
    queryProduto.bindValue(":idOrcamento", id);
  }

  if (tipo == Tipo::Venda) {
    query.prepare("SELECT idLoja, idUsuario, idProfissional, idEnderecoFaturamento, idEnderecoEntrega, idCliente, idOrcamento, data, subTotalLiq, subTotalBru, descontoPorc, frete, total, "
                  "prazoEntrega, observacao FROM venda WHERE idVenda = :idVenda");
    query.bindValue(":idVenda", id);

    queryProduto.prepare(
        "SELECT idProduto, fornecedor, codComercial, formComercial, produto, obs, prcUnitario, desconto, quant, un, parcial, parcialDesc FROM venda_has_produto WHERE idVenda = :idVenda");
    queryProduto.bindValue(":idVenda", id);
  }

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dados da venda/orçamento: " + query.lastError().text()); }

  //------------------------------------------------------------------------

  if (not queryProduto.exec() or not queryProduto.first()) { return qApp->enqueueError(false, "Erro buscando dados dos produtos: " + query.lastError().text()); } //TODO: V778 http://www.viva64.com/en/V778 Two similar code fragments were found. Perhaps, this is a typo and 'queryProduto' variable should be used instead of 'query'.  if (not queryProduto.exec() or not queryProduto.first()) { return qApp->enqueueError(false, "Erro buscando dados dos produtos: " + query.lastError().text()); }

  //------------------------------------------------------------------------

  queryCliente.prepare("SELECT nome_razao, email, cpf, cnpj, pfpj, tel, telCel FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", query.value("idCliente"));

  if (not queryCliente.exec() or not queryCliente.first()) { return qApp->enqueueError(false, "Erro buscando cliente: " + queryCliente.lastError().text()); }

  //------------------------------------------------------------------------

  queryEndEnt.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndEnt.bindValue(":idEndereco", query.value("idEnderecoEntrega"));

  if (not queryEndEnt.exec() or not queryEndEnt.first()) { return qApp->enqueueError(false, "Erro buscando dados do endereço entrega: " + queryEndEnt.lastError().text()); }

  //------------------------------------------------------------------------

  queryEndFat.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndFat.bindValue(":idEndereco", query.value(tipo == Tipo::Venda ? "idEnderecoFaturamento" : "idEnderecoEntrega"));

  if (not queryEndFat.exec() or not queryEndFat.first()) { return qApp->enqueueError(false, "Erro buscando dados do endereço: " + queryEndFat.lastError().text()); }

  //------------------------------------------------------------------------

  queryProfissional.prepare("SELECT nome_razao, tel, email FROM profissional WHERE idProfissional = :idProfissional");
  queryProfissional.bindValue(":idProfissional", query.value("idProfissional"));

  if (not queryProfissional.exec() or not queryProfissional.first()) { return qApp->enqueueError(false, "Erro buscando profissional: " + queryProfissional.lastError().text()); }

  //------------------------------------------------------------------------

  queryVendedor.prepare("SELECT nome, email FROM usuario WHERE idUsuario = :idUsuario");
  queryVendedor.bindValue(":idUsuario", query.value("idUsuario"));

  if (not queryVendedor.exec() or not queryVendedor.first()) { return qApp->enqueueError(false, "Erro buscando vendedor: " + queryVendedor.lastError().text()); }

  //------------------------------------------------------------------------

  queryLoja.prepare("SELECT tel, tel2 FROM loja WHERE idLoja = :idLoja");
  queryLoja.bindValue(":idLoja", query.value("idLoja"));

  if (not queryLoja.exec() or not queryLoja.first()) { return qApp->enqueueError(false, "Erro buscando loja: " + queryLoja.lastError().text()); }

  //------------------------------------------------------------------------

  queryLojaEnd.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep, uf FROM loja_has_endereco WHERE idLoja = :idLoja");
  queryLojaEnd.bindValue(":idLoja", query.value("idLoja"));

  if (not queryLojaEnd.exec() or not queryLojaEnd.first()) { return qApp->enqueueError(false, "Erro buscando endereço loja: " + queryLojaEnd.lastError().text()); }

  return true;
}
