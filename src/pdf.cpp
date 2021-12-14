#include "pdf.h"

#include "application.h"

#if __has_include("lrreportengine.h")
#include "lrreportengine.h"

#include "file.h"
#include "user.h"

#include <QDesktopServices>
#include <QDir>
#endif

#include <QSqlError>

PDF::PDF(const QString &id, const Tipo tipo, QWidget *parent) : id(id), parent(parent), tipo(tipo) {
  Q_UNUSED(this->parent)

  modelItem.setTable((tipo == Tipo::Orcamento ? "orcamento" : "venda") + QString("_has_produto"));

  modelItem.setFilter(tipo == Tipo::Orcamento ? "idOrcamento = '" + id + "'" : "idVenda = '" + id + "'");

  modelItem.select();
}

void PDF::gerarPdf() {
#if __has_include("lrreportengine.h")
  const QString folder = tipo == Tipo::Orcamento ? "User/OrcamentosFolder" : "User/VendasFolder";

  const QString folderKey = User::getSetting(folder).toString();

  if (folderKey.isEmpty()) { throw RuntimeError("Não há uma pasta definida para salvar PDF/Excel. Por favor escolha uma nas configurações do ERP!", parent); }

  setQuerys();

  LimeReport::ReportEngine report;
  auto *dataManager = report.dataManager();

  dataManager->addModel(tipo == Tipo::Orcamento ? "orcamento" : "venda", &modelItem, false);

  const QString modelo = QDir::currentPath() + "/modelos/" + ((tipo == Tipo::Orcamento) ? "orcamento" : "venda") + ".lrxml";

  if (not report.loadFromFile(modelo)) { throw RuntimeException("Não encontrou o modelo de impressão!", parent); }

  dataManager->setReportVariable("Loja", queryLoja.value("descricao"));
  dataManager->setReportVariable("EnderecoLoja", queryLojaEnd.value("logradouro").toString() + ", " + queryLojaEnd.value("numero").toString() + "\n" + queryLojaEnd.value("bairro").toString() + "\n" +
                                                     queryLojaEnd.value("cidade").toString() + " - " + queryLojaEnd.value("uf").toString() + " - CEP: " + queryLojaEnd.value("cep").toString() + "\n" +
                                                     queryLoja.value("tel").toString() + " - " + queryLoja.value("tel2").toString());

  dataManager->setReportVariable("Data", query.value("data").toDate().toString("dd-MM-yyyy"));
  dataManager->setReportVariable("Cliente", queryCliente.value("nome_razao"));
  const QString cpfcnpj = (queryCliente.value("pfpj") == "PF") ? "CPF: " : "CNPJ: ";
  dataManager->setReportVariable("CPFCNPJ", cpfcnpj + queryCliente.value((queryCliente.value("pfpj") == "PF") ? "cpf" : "cnpj").toString());
  dataManager->setReportVariable("EmailCliente", queryCliente.value("email"));
  dataManager->setReportVariable("Tel1", queryCliente.value("tel"));
  dataManager->setReportVariable("Tel2", queryCliente.value("telCel"));

  if (tipo == Tipo::Venda and queryEndFat.size() > 0) {
    const QString endereco = query.value("idEnderecoFaturamento").toInt() == 1
                                 ? "NÃO HÁ/RETIRA"
                                 : queryEndFat.value("logradouro").toString() + " - " + queryEndFat.value("numero").toString() + " - " + queryEndFat.value("complemento").toString() + " - " +
                                       queryEndFat.value("bairro").toString() + " - " + queryEndFat.value("cidade").toString() + " - " + queryEndFat.value("uf").toString();

    dataManager->setReportVariable("EndFiscal", endereco);
    dataManager->setReportVariable("CEPFiscal", queryEndFat.value("cep"));
  }

  dataManager->setReportVariable("CEPEntrega", queryEndEnt.value("cep"));

  if (queryEndEnt.size() > 0) {
    const QString endereco = query.value("idEnderecoEntrega").toInt() == 1
                                 ? "NÃO HÁ/RETIRA"
                                 : queryEndEnt.value("logradouro").toString() + " - " + queryEndEnt.value("numero").toString() + " - " + queryEndEnt.value("complemento").toString() + " - " +
                                       queryEndEnt.value("bairro").toString() + " - " + queryEndEnt.value("cidade").toString() + " - " + queryEndEnt.value("uf").toString();

    dataManager->setReportVariable("EndEntrega", endereco);

    if (tipo == Tipo::Orcamento) { dataManager->setReportVariable("EndFiscal", endereco); }
  }

  QString profissional = queryProfissional.value("nome_razao").toString();
  if (not profissional.isEmpty() and mostrarRT) { profissional += R"( <span style="color: red"><strong>)" + query.value("rt").toString() + "%</strong>"; }
  dataManager->setReportVariable("Profissional", profissional.isEmpty() ? "NÃO HÁ" : profissional);
  dataManager->setReportVariable("EmailProfissional", queryProfissional.value("email"));
  dataManager->setReportVariable("Vendedor", queryVendedor.value("nome"));
  dataManager->setReportVariable("EmailVendedor", queryVendedor.value("email"));

  QLocale locale;

  dataManager->setReportVariable("Soma", locale.toString(query.value("subTotalLiq").toDouble(), 'f', 2));
  dataManager->setReportVariable("Desconto", "R$ " + locale.toString(query.value("descontoReais").toDouble(), 'f', 2) + " (" + locale.toString(query.value("descontoPorc").toDouble(), 'f', 2) + "%)");
  double value = query.value("total").toDouble() - query.value("frete").toDouble();
  dataManager->setReportVariable("Total", locale.toString(value, 'f', 2));
  dataManager->setReportVariable("Frete", locale.toString(query.value("frete").toDouble(), 'f', 2));
  dataManager->setReportVariable("TotalFinal", locale.toString(query.value("total").toDouble(), 'f', 2));
  dataManager->setReportVariable("Observacao", query.value("observacao").toString().replace("\n", " "));

  if (tipo == Tipo::Orcamento) {
    dataManager->setReportVariable("Orcamento", id);
    dataManager->setReportVariable("Validade", query.value("validade").toString() + " dias");
  }

  if (tipo == Tipo::Venda) {
    dataManager->setReportVariable("Pedido", id);
    dataManager->setReportVariable("Orcamento", query.value("idOrcamento"));
    dataManager->setReportVariable("PrazoEntrega", query.value("prazoEntrega").toString() + " dias");

    const QString pgtQuery = "SELECT ANY_VALUE(tipo) AS tipo, COUNT(valor) AS parcelas, ANY_VALUE(valor) AS valor, ANY_VALUE(dataPagamento) AS dataPagamento, ANY_VALUE(observacao) AS observacao FROM "
                             "conta_a_receber_has_pagamento WHERE idVenda = '" +
                             id + "' AND tipo LIKE '%1%' AND tipo NOT IN ('%1. Comissão', '%1. Taxa Cartão') AND status NOT IN ('CANCELADO', 'SUBSTITUIDO')";

    for (int i = 1; i <= 5; ++i) {
      SqlQuery queryPgt;

      const QString current = QString::number(i);

      if (not queryPgt.exec(pgtQuery.arg(current))) { throw RuntimeException("Erro buscando pagamento " + current + ": " + queryPgt.lastError().text(), parent); }

      if (not queryPgt.first()) { throw RuntimeException("Pagamento não encontrado!"); }

      if (qFuzzyIsNull(queryPgt.value("valor").toDouble())) { continue; }

      const QString pagEm = (queryPgt.value("parcelas").toInt() == 1) ? " - pag. em: " : " - 1° pag. em: ";
      const QString observacao = queryPgt.value("observacao").toString();

      const QString pgt = queryPgt.value("tipo").toString() + " - " + queryPgt.value("parcelas").toString() + "x de R$ " + locale.toString(queryPgt.value("valor").toDouble(), 'f', 2) + pagEm +
                          queryPgt.value("dataPagamento").toDate().toString("dd-MM-yyyy") + (observacao.isEmpty() ? "" : " - " + observacao);

      dataManager->setReportVariable("FormaPagamento" + current, pgt);
    }
  }

  QString fileName = id + "-" + queryVendedor.value("nome").toString().split(" ").first() + "-" + queryCliente.value("nome_razao").toString().replace("/", "-") + ".pdf";
  fileName.remove(R"(\)").remove("/").remove(":").remove("*").remove("?").remove(R"(")").remove("<").remove(">").remove("|");

  fileName = folderKey + "/" + fileName;

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeError("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString(), parent); }

  file.close();

  if (not report.printToPDF(fileName)) { throw RuntimeException("Erro gerando PDF: " + report.lastError()); }

  if (not QDesktopServices::openUrl(QUrl::fromLocalFile(fileName))) { throw RuntimeException("Erro abrindo arquivo: " + QDir::currentPath() + fileName); }

  qApp->enqueueInformation("Arquivo salvo como " + fileName, parent);
#else
  qApp->enqueueWarning("LimeReport desativado!");
#endif
}

void PDF::setQuerys() {
  if (tipo == Tipo::Orcamento) {
    query.prepare("SELECT idCliente, idProfissional, idUsuario, idLoja, data, validade, idEnderecoFaturamento, idEnderecoEntrega, subTotalLiq, descontoPorc, descontoReais, frete, total, observacao, "
                  "prazoEntrega FROM orcamento WHERE idOrcamento = :idOrcamento");
    query.bindValue(":idOrcamento", id);
  }

  if (tipo == Tipo::Venda) {
    query.prepare("SELECT idCliente, idProfissional, idUsuario, idLoja, idOrcamento, data, idEnderecoFaturamento, idEnderecoEntrega, subTotalLiq, descontoPorc, descontoReais, frete, total, "
                  "observacao, prazoEntrega, rt FROM venda WHERE idVenda = :idVenda");
    query.bindValue(":idVenda", id);
  }

  if (not query.exec()) { throw RuntimeException("Erro buscando dados da venda/orçamento: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados da venda/orçamento não encontrados para id: " + id); }

  //------------------------------------------------------------------------

  queryCliente.prepare("SELECT nome_razao, pfpj, cpf, cnpj, email, tel, telCel FROM cliente WHERE idCliente = :idCliente");
  queryCliente.bindValue(":idCliente", query.value("idCliente"));

  if (not queryCliente.exec()) { throw RuntimeException("Erro buscando cliente: " + queryCliente.lastError().text()); }

  if (not queryCliente.first()) { throw RuntimeException("Dados do cliente não encontrado para id: " + query.value("idCliente").toString()); }

  //------------------------------------------------------------------------

  queryEndEnt.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
  queryEndEnt.bindValue(":idEndereco", query.value("idEnderecoEntrega"));

  if (not queryEndEnt.exec()) { throw RuntimeException("Erro buscando endereço: " + queryEndEnt.lastError().text()); }

  if (not queryEndEnt.first()) { throw RuntimeException("Dados do endereço não encontrados para id: " + query.value("idEnderecoEntrega").toString()); }

  //------------------------------------------------------------------------

  if (tipo == Tipo::Venda) {
    queryEndFat.prepare("SELECT logradouro, numero, complemento, bairro, cidade, uf, cep FROM cliente_has_endereco WHERE idEndereco = :idEndereco");
    queryEndFat.bindValue(":idEndereco", query.value("idEnderecoFaturamento"));

    if (not queryEndFat.exec()) { throw RuntimeException("Erro buscando dados do endereço: " + queryEndFat.lastError().text()); }

    if (not queryEndFat.first()) { throw RuntimeException("Dados do endereço não encontrados para id: " + query.value("idEnderecoFaturamento").toString()); }
  }

  //------------------------------------------------------------------------

  queryProfissional.prepare("SELECT nome_razao, tel, email FROM profissional WHERE idProfissional = :idProfissional");
  queryProfissional.bindValue(":idProfissional", query.value("idProfissional"));

  if (not queryProfissional.exec()) { throw RuntimeException("Erro buscando profissional: " + queryProfissional.lastError().text()); }

  if (not queryProfissional.first()) { throw RuntimeException("Dados do profissional não encontrados para id: " + query.value("idProfissional").toString()); }

  //------------------------------------------------------------------------

  queryVendedor.prepare("SELECT nome, email FROM usuario WHERE idUsuario = :idUsuario");
  queryVendedor.bindValue(":idUsuario", query.value("idUsuario"));

  if (not queryVendedor.exec()) { throw RuntimeException("Erro buscando vendedor: " + queryVendedor.lastError().text()); }

  if (not queryVendedor.first()) { throw RuntimeException("Dados do vendedor não encontrados para id: " + query.value("idUsuario").toString()); }

  //------------------------------------------------------------------------

  queryLoja.prepare("SELECT descricao, tel, tel2 FROM loja WHERE idLoja = :idLoja");
  queryLoja.bindValue(":idLoja", query.value("idLoja"));

  if (not queryLoja.exec()) { throw RuntimeException("Erro buscando loja: " + queryLoja.lastError().text()); }

  if (not queryLoja.first()) { throw RuntimeException("Dados da loja não encontrados para id: " + query.value("idLoja").toString()); }

  //------------------------------------------------------------------------

  queryLojaEnd.prepare("SELECT logradouro, numero, bairro, cidade, uf, cep FROM loja_has_endereco WHERE idLoja = :idLoja");
  queryLojaEnd.bindValue(":idLoja", query.value("idLoja"));

  if (not queryLojaEnd.exec()) { throw RuntimeException("Erro buscando loja endereço: " + queryLojaEnd.lastError().text()); }

  if (not queryLojaEnd.first()) { throw RuntimeException("Endereço da loja não encontrado para id: " + query.value("idLoja").toString()); }
}
