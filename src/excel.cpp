#include "excel.h"

#include "application.h"
#include "file.h"
#include "user.h"

#include <QDesktopServices>
#include <QDir>
#include <QSqlError>
#include <QUrl>

Excel::Excel(const QString &id, const Tipo tipo, QWidget *parent) : id(id), parent(parent), tipo(tipo) {}

void Excel::hideUnusedRows(QXlsx::Document &xlsx) {
  for (int row = queryProduto.size() + 12; row < 398; ++row) { xlsx.setRowHidden(row, true); }
}

void Excel::gerarExcel() {
  // TODO: dear god, divide this into smaller funcs

  const QString folder = (tipo == Tipo::Orcamento) ? "User/OrcamentosFolder" : "User/VendasFolder";
  const QString folderKey = User::getSetting(folder).toString();

  if (folderKey.isEmpty()) { throw RuntimeError("Não há uma pasta definida para salvar PDF/Excel. Por favor escolha uma nas configurações do ERP!"); }

  const QString arquivoModelo = QDir::currentPath() + "/modelos/pedido.xlsx";

  File modelo(arquivoModelo);

  if (not modelo.exists()) { throw RuntimeException("Não encontrou o modelo do Excel!"); }

  setQuerys();

  if (anexoCompra) {
    fileName = customFileName;
  } else {
    fileName = id + "-" + queryVendedor.value("nome").toString().split(" ").first() + "-" + queryCliente.value("nome_razao").toString().replace("/", "-") + ".xlsx";
    fileName.remove(R"(\)").remove("/").remove(":").remove("*").remove("?").remove(R"(")").remove("<").remove(">").remove("|");
    fileName = folderKey + "/" + fileName;
  }

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeError("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

  file.close();

  QLocale locale;

  QXlsx::Document xlsx(arquivoModelo, parent);

  xlsx.currentWorksheet()->setFitToPage(true);
  xlsx.currentWorksheet()->setFitToHeight(true);
  xlsx.currentWorksheet()->setOrientation(QXlsx::Worksheet::Orientation::Horizontal);

  const QString endLoja = queryLojaEnd.value("logradouro").toString() + ", " + queryLojaEnd.value("numero").toString() + " " + queryLojaEnd.value("complemento").toString() + " - " +
                          queryLojaEnd.value("bairro").toString() + "\n" + queryLojaEnd.value("cidade").toString() + " - " + queryLojaEnd.value("uf").toString() +
                          " - CEP: " + queryLojaEnd.value("cep").toString() + "\n" + queryLoja.value("tel").toString() + " - " + queryLoja.value("tel2").toString();

  const QString endEntrega = queryEndEnt.value("logradouro").toString().isEmpty()
                                 ? "NÃO HÁ/RETIRA"
                                 : queryEndEnt.value("logradouro").toString() + " " + queryEndEnt.value("numero").toString() + " " + queryEndEnt.value("complemento").toString() + " - " +
                                       queryEndEnt.value("bairro").toString() + ", " + queryEndEnt.value("cidade").toString();

  const QString endFat = queryEndFat.value("logradouro").toString().isEmpty()
                             ? "NÃO HÁ/RETIRA"
                             : queryEndFat.value("logradouro").toString() + " " + queryEndFat.value("numero").toString() + " " + queryEndFat.value("complemento").toString() + " - " +
                                   queryEndFat.value("bairro").toString() + ", " + queryEndFat.value("cidade").toString();

  xlsx.write("A5", endLoja);

  if (tipo == Tipo::Venda) {
    if (anexoCompra) {
      xlsx.write("C2", "Pedido:");
      xlsx.write("D2", "O.C. " + QString::number(ordemCompra) + " " + id);
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
  if (mostrarRT) { xlsx.write("G7", query.value("RT").toDouble() / 100); }
  xlsx.write("D8", queryVendedor.value("nome"));
  xlsx.write("F8", queryVendedor.value("email"));
  xlsx.write("M2", query.value("data").toDateTime().toString("dd/MM/yyyy hh:mm"));
  xlsx.write("M3", queryCliente.value(queryCliente.value("pfpj") == "PF" ? "cpf" : "cnpj"));
  xlsx.write("M4", queryCliente.value("tel"));
  xlsx.write("J4", queryCliente.value("telCel"));
  xlsx.write("M5", queryEndFat.value("cep"));
  xlsx.write("M6", queryEndEnt.value("cep"));
  xlsx.write("I7", queryProfissional.value("tel"));
  xlsx.write("L7", queryProfissional.value("email"));

  const double subLiq = query.value("subTotalLiq").toDouble();
  const double subBru = query.value("subTotalBru").toDouble();
  const double desconto = query.value("descontoPorc").toDouble();
  // TODO: 0no lugar de calcular valores, usar os do BD
  xlsx.write("N400", subLiq > subBru ? "R$ " + locale.toString(subLiq, 'f', 2) : "R$ " + locale.toString(subBru, 'f', 2) + " (R$ " + locale.toString(subLiq, 'f', 2) + ")"); // soma
  xlsx.write("N401", locale.toString(desconto, 'f', 2) + "%");                                                                                                               // desconto
  xlsx.write("N402", "R$ " + locale.toString(subLiq - (desconto / 100. * subLiq), 'f', 2));                                                                                  // total
  xlsx.write("N403", "R$ " + locale.toString(query.value("frete").toDouble(), 'f', 2));                                                                                      // frete
  xlsx.write("N404", "R$ " + locale.toString(query.value("total").toDouble(), 'f', 2));                                                                                      // total final
  xlsx.write("B400", query.value("prazoEntrega").toString() + " dias");

  const QString pgtQuery = "SELECT ANY_VALUE(tipo) AS tipo, COUNT(valor) AS parcelas, ANY_VALUE(valor) AS valor, ANY_VALUE(dataPagamento) AS dataPagamento, ANY_VALUE(observacao) AS observacao FROM "
                           "conta_a_receber_has_pagamento WHERE idVenda = '" +
                           id + "' AND tipo LIKE '%1%' AND tipo NOT IN ('%1. Comissão', '%1. Taxa Cartão') AND status NOT IN ('CANCELADO', 'SUBSTITUIDO')";

  for (int i = 1; i <= 5; ++i) {
    const QString current = QString::number(i);

    SqlQuery queryPgt;

    if (not queryPgt.exec(pgtQuery.arg(current))) { throw RuntimeException("Erro buscando pagamento " + current + ": " + queryPgt.lastError().text()); }

    if (not queryPgt.first()) { throw RuntimeException("Pagamento não encontrado!"); }

    if (qFuzzyIsNull(queryPgt.value("valor").toDouble())) { continue; }

    const QString pagEm = (queryPgt.value("parcelas").toInt() == 1 ? " - pag. em: " : " - 1° pag. em: ");
    const QString observacao = queryPgt.value("observacao").toString();

    const QString pgt = queryPgt.value("tipo").toString() + " - " + queryPgt.value("parcelas").toString() + "x de R$ " + locale.toString(queryPgt.value("valor").toDouble(), 'f', 2) + pagEm +
                        queryPgt.value("dataPagamento").toDate().toString("dd-MM-yyyy") + (observacao.isEmpty() ? "" : " - " + observacao);

    xlsx.write("B" + QString::number(400 + i), pgt);
  }

  xlsx.write("B406", query.value("observacao").toString().replace("\n", " "));

  // TODO: 5refator this to start at 12
  int row = 0;

  SqlQuery queryUi; // TODO: put this query in 'setQuerys'?
  queryUi.prepare("SELECT ui FROM produto WHERE idProduto = :idProduto");

  do {
    queryUi.bindValue(":idProduto", queryProduto.value("idProduto"));

    if (not queryUi.exec()) { throw RuntimeException("Erro buscando dados do produto: " + queryUi.lastError().text()); }

    if (not queryUi.first()) { throw RuntimeException("Dados não encontrados do produto com id: '" + queryProduto.value("idProduto").toString() + "'"); }

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

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Erro ao salvar arquivo!"); }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName, parent);
}

void Excel::setQuerys() {
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
                  "prazoEntrega, observacao, rt FROM venda WHERE idVenda = :idVenda");
    query.bindValue(":idVenda", id);

    queryProduto.prepare(
        "SELECT idProduto, fornecedor, codComercial, formComercial, produto, obs, prcUnitario, desconto, quant, un, parcial, parcialDesc FROM venda_has_produto WHERE idVenda = :idVenda");
    queryProduto.bindValue(":idVenda", id);
  }

  if (not query.exec()) { throw RuntimeException("Erro buscando dados da venda/orçamento: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados da venda/orçamento: '" + id + "'"); }

  //------------------------------------------------------------------------

  if (not queryProduto.exec()) { throw RuntimeException("Erro buscando dados dos produtos: " + queryProduto.lastError().text()); }

  if (not queryProduto.first()) { throw RuntimeException("Dados não encontrados da Venda: '" + id + "'"); }

  //------------------------------------------------------------------------

  queryCliente.prepare("SELECT nome_razao, email, cpf, cnpj, pfpj, tel, telCel FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", query.value("idCliente"));

  if (not queryCliente.exec()) { throw RuntimeException("Erro buscando cliente: " + queryCliente.lastError().text()); }

  if (not queryCliente.first()) { throw RuntimeException("Dados não encontrados do cliente com id: '" + query.value("idCliente").toString() + "'"); }

  //------------------------------------------------------------------------

  queryEndEnt.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndEnt.bindValue(":idEndereco", query.value("idEnderecoEntrega"));

  if (not queryEndEnt.exec()) { throw RuntimeException("Erro buscando dados do endereço entrega: " + queryEndEnt.lastError().text()); }

  if (not queryEndEnt.first()) { throw RuntimeException("Dados não encontrados do endereço com id: '" + query.value("idEnderecoEntrega").toString() + "'"); }

  //------------------------------------------------------------------------

  queryEndFat.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndFat.bindValue(":idEndereco", query.value(tipo == Tipo::Venda ? "idEnderecoFaturamento" : "idEnderecoEntrega"));

  if (not queryEndFat.exec()) { throw RuntimeException("Erro buscando dados do endereço: " + queryEndFat.lastError().text()); }

  if (not queryEndFat.first()) {
    throw RuntimeException("Dados não encontrados do endereço com id: '" + query.value(tipo == Tipo::Venda ? "idEnderecoFaturamento" : "idEnderecoEntrega").toString() + "'");
  }

  //------------------------------------------------------------------------

  queryProfissional.prepare("SELECT nome_razao, tel, email FROM profissional WHERE idProfissional = :idProfissional");
  queryProfissional.bindValue(":idProfissional", query.value("idProfissional"));

  if (not queryProfissional.exec()) { throw RuntimeException("Erro buscando profissional: " + queryProfissional.lastError().text()); }

  if (not queryProfissional.first()) { throw RuntimeException("Dados não encontrados do profissional com id: '" + query.value("idProfissional").toString() + "'"); }

  //------------------------------------------------------------------------

  queryVendedor.prepare("SELECT nome, email FROM usuario WHERE idUsuario = :idUsuario");
  queryVendedor.bindValue(":idUsuario", query.value("idUsuario"));

  if (not queryVendedor.exec()) { throw RuntimeException("Erro buscando vendedor: " + queryVendedor.lastError().text()); }

  if (not queryVendedor.first()) { throw RuntimeException("Dados não encontrados do vendedor com id: '" + query.value("idUsuario").toString() + "'"); }

  //------------------------------------------------------------------------

  queryLoja.prepare("SELECT tel, tel2 FROM loja WHERE idLoja = :idLoja");
  queryLoja.bindValue(":idLoja", query.value("idLoja"));

  if (not queryLoja.exec()) { throw RuntimeException("Erro buscando loja: " + queryLoja.lastError().text()); }

  if (not queryLoja.first()) { throw RuntimeException("Dados não encontrados da loja com id: '" + query.value("idLoja").toString() + "'"); }

  //------------------------------------------------------------------------

  queryLojaEnd.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep, uf FROM loja_has_endereco WHERE idLoja = :idLoja AND desativado = FALSE");
  queryLojaEnd.bindValue(":idLoja", query.value("idLoja"));

  if (not queryLojaEnd.exec()) { throw RuntimeException("Erro buscando endereço loja: " + queryLojaEnd.lastError().text()); }

  if (not queryLojaEnd.first()) { throw RuntimeException("Endereço não encontrado da loja com id: '" + query.value("idLoja").toString() + "'"); }
}
