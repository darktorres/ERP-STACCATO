#include "devolucao.h"
#include "ui_devolucao.h"

#include "application.h"
#include "checkboxdelegate.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "usersession.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <cmath>

Devolucao::Devolucao(const QString &idVenda, QWidget *parent) : QDialog(parent), idVenda(idVenda), ui(new Ui::Devolucao) {
  ui->setupUi(this);

  setConnections();

  setWindowFlags(Qt::Window);

  setWindowTitle(idVenda);
  setupTables();
}

Devolucao::~Devolucao() { delete ui; }

void Devolucao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->doubleSpinBoxCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCaixas_valueChanged, connectionType);
  connect(ui->doubleSpinBoxQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxQuant_valueChanged, connectionType);
  connect(ui->doubleSpinBoxCredito, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCredito_valueChanged, connectionType);
  connect(ui->doubleSpinBoxPorcentagem, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxPorcentagem_valueChanged, connectionType);
  connect(ui->pushButtonDevolverItem, &QPushButton::clicked, this, &Devolucao::on_pushButtonDevolverItem_clicked, connectionType);
  connect(ui->tableProdutos, &TableView::clicked, this, &Devolucao::on_tableProdutos_clicked, connectionType);
}

void Devolucao::unsetConnections() {
  disconnect(ui->doubleSpinBoxCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCaixas_valueChanged);
  disconnect(ui->doubleSpinBoxQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxQuant_valueChanged);
  disconnect(ui->doubleSpinBoxCredito, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCredito_valueChanged);
  disconnect(ui->doubleSpinBoxPorcentagem, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxPorcentagem_valueChanged);
  disconnect(ui->pushButtonDevolverItem, &QPushButton::clicked, this, &Devolucao::on_pushButtonDevolverItem_clicked);
  disconnect(ui->tableProdutos, &TableView::clicked, this, &Devolucao::on_tableProdutos_clicked);
}

bool Devolucao::determinarIdDevolucao() {
  QSqlQuery query;
  query.prepare("SELECT MAX(idVenda) AS id FROM venda WHERE idVenda LIKE :idVenda AND MONTH(data) = MONTH(NOW()) HAVING MAX(idVenda) IS NOT NULL");
  query.bindValue(":idVenda", idVenda + "D%");

  if (not query.exec()) { return qApp->enqueueError(false, "Erro verificando se existe devolução: " + query.lastError().text(), this); }

  if (query.first()) {
    idDevolucao = query.value("id").toString();
    return true;
  }

  if (not criarDevolucao()) { return false; }

  return true;
}

void Devolucao::setupTables() {
  modelProdutos2.setTable("venda_has_produto2");

  modelProdutos2.setHeaderData("status", "Status");
  modelProdutos2.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos2.setHeaderData("produto", "Produto");
  modelProdutos2.setHeaderData("obs", "Obs.");
  modelProdutos2.setHeaderData("lote", "Lote");
  modelProdutos2.setHeaderData("caixas", "Caixas");
  modelProdutos2.setHeaderData("quant", "Quant.");
  modelProdutos2.setHeaderData("un", "Un.");
  modelProdutos2.setHeaderData("quantCaixa", "Quant./Cx.");
  modelProdutos2.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos2.setHeaderData("formComercial", "Form. Com.");
  modelProdutos2.setHeaderData("total", "Total");

  modelProdutos2.setFilter("idVenda = '" + idVenda + "' AND status != 'DEVOLVIDO'");

  if (not modelProdutos2.select()) { return; }

  ui->tableProdutos->setModel(&modelProdutos2);

  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("idVendaProdutoFK");
  ui->tableProdutos->hideColumn("idRelacionado");
  ui->tableProdutos->hideColumn("selecionado");
  ui->tableProdutos->hideColumn("entregou");
  ui->tableProdutos->hideColumn("recebeu");
  ui->tableProdutos->hideColumn("statusOriginal");
  ui->tableProdutos->hideColumn("idCompra");
  ui->tableProdutos->hideColumn("idNFeSaida");
  ui->tableProdutos->hideColumn("idNFeFutura");
  ui->tableProdutos->hideColumn("idVenda");
  ui->tableProdutos->hideColumn("idLoja");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("prcUnitario");
  ui->tableProdutos->hideColumn("descUnitario");
  ui->tableProdutos->hideColumn("parcial");
  ui->tableProdutos->hideColumn("desconto");
  ui->tableProdutos->hideColumn("parcialDesc");
  ui->tableProdutos->hideColumn("descGlobal");
  ui->tableProdutos->hideColumn("mostrarDesconto");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("reposicaoEntrega");
  ui->tableProdutos->hideColumn("reposicaoReceb");
  ui->tableProdutos->hideColumn("dataPrevCompra");
  ui->tableProdutos->hideColumn("dataRealCompra");
  ui->tableProdutos->hideColumn("dataPrevConf");
  ui->tableProdutos->hideColumn("dataRealConf");
  ui->tableProdutos->hideColumn("dataPrevFat");
  ui->tableProdutos->hideColumn("dataRealFat");
  ui->tableProdutos->hideColumn("dataPrevColeta");
  ui->tableProdutos->hideColumn("dataRealColeta");
  ui->tableProdutos->hideColumn("dataPrevReceb");
  ui->tableProdutos->hideColumn("dataRealReceb");
  ui->tableProdutos->hideColumn("dataPrevEnt");
  ui->tableProdutos->hideColumn("dataRealEnt");

  ui->tableProdutos->setItemDelegate(new DoubleDelegate(this));

  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("total", new ReaisDelegate(this));

  //--------------------------------------------------------------

  modelDevolvidos1.setTable("venda_has_produto");

  modelDevolvidos1.setHeaderData("status", "Status");
  modelDevolvidos1.setHeaderData("statusOriginal", "Status Original");
  modelDevolvidos1.setHeaderData("fornecedor", "Fornecedor");
  modelDevolvidos1.setHeaderData("produto", "Produto");
  modelDevolvidos1.setHeaderData("obs", "Obs.");
  modelDevolvidos1.setHeaderData("lote", "Lote");
  modelDevolvidos1.setHeaderData("prcUnitario", "R$ Unit.");
  modelDevolvidos1.setHeaderData("caixas", "Caixas");
  modelDevolvidos1.setHeaderData("quant", "Quant.");
  modelDevolvidos1.setHeaderData("un", "Un.");
  modelDevolvidos1.setHeaderData("quantCaixa", "Quant./Cx.");
  modelDevolvidos1.setHeaderData("codComercial", "Cód. Com.");
  modelDevolvidos1.setHeaderData("formComercial", "Form. Com.");
  modelDevolvidos1.setHeaderData("total", "Total");

  modelDevolvidos1.setFilter("idVenda LIKE '" + idVenda + "D%'");

  if (not modelDevolvidos1.select()) { return; }

  ui->tableDevolvidos->setModel(&modelDevolvidos1);

  ui->tableDevolvidos->hideColumn("idVendaProduto1");
  ui->tableDevolvidos->hideColumn("idRelacionado");
  ui->tableDevolvidos->hideColumn("selecionado");
  ui->tableDevolvidos->hideColumn("entregou");
  ui->tableDevolvidos->hideColumn("recebeu");
  ui->tableDevolvidos->hideColumn("statusOriginal");
  ui->tableDevolvidos->hideColumn("idCompra");
  ui->tableDevolvidos->hideColumn("idNFeSaida");
  ui->tableDevolvidos->hideColumn("idNFeFutura");
  ui->tableDevolvidos->hideColumn("idVenda");
  ui->tableDevolvidos->hideColumn("idLoja");
  ui->tableDevolvidos->hideColumn("idProduto");
  ui->tableDevolvidos->hideColumn("descUnitario");
  ui->tableDevolvidos->hideColumn("parcial");
  ui->tableDevolvidos->hideColumn("desconto");
  ui->tableDevolvidos->hideColumn("parcialDesc");
  ui->tableDevolvidos->hideColumn("descGlobal");
  ui->tableDevolvidos->hideColumn("mostrarDesconto");
  ui->tableDevolvidos->hideColumn("estoque");
  ui->tableDevolvidos->hideColumn("promocao");
  ui->tableDevolvidos->hideColumn("reposicaoEntrega");
  ui->tableDevolvidos->hideColumn("reposicaoReceb");
  ui->tableDevolvidos->hideColumn("dataPrevCompra");
  ui->tableDevolvidos->hideColumn("dataRealCompra");
  ui->tableDevolvidos->hideColumn("dataPrevConf");
  ui->tableDevolvidos->hideColumn("dataRealConf");
  ui->tableDevolvidos->hideColumn("dataPrevFat");
  ui->tableDevolvidos->hideColumn("dataRealFat");
  ui->tableDevolvidos->hideColumn("dataPrevColeta");
  ui->tableDevolvidos->hideColumn("dataRealColeta");
  ui->tableDevolvidos->hideColumn("dataPrevReceb");
  ui->tableDevolvidos->hideColumn("dataRealReceb");
  ui->tableDevolvidos->hideColumn("dataPrevEnt");
  ui->tableDevolvidos->hideColumn("dataRealEnt");

  ui->tableDevolvidos->setItemDelegate(new DoubleDelegate(this));

  ui->tableDevolvidos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableDevolvidos->setItemDelegateForColumn("total", new ReaisDelegate(this));

  //--------------------------------------------------------------

  modelPagamentos.setTable("conta_a_receber_has_pagamento");

  modelPagamentos.setHeaderData("tipo", "Tipo");
  modelPagamentos.setHeaderData("parcela", "Parcela");
  modelPagamentos.setHeaderData("valor", "R$");
  modelPagamentos.setHeaderData("dataPagamento", "Data");
  modelPagamentos.setHeaderData("observacao", "Obs.");
  modelPagamentos.setHeaderData("status", "Status");
  modelPagamentos.setHeaderData("representacao", "Representação");

  modelPagamentos.setFilter("idVenda LIKE '" + idVenda + "D%'");

  if (not modelPagamentos.select()) { return; }

  ui->tablePagamentos->setModel(&modelPagamentos);

  ui->tablePagamentos->hideColumn("nfe");
  ui->tablePagamentos->hideColumn("idVenda");
  ui->tablePagamentos->hideColumn("idLoja");
  ui->tablePagamentos->hideColumn("idPagamento");
  ui->tablePagamentos->hideColumn("dataEmissao");
  ui->tablePagamentos->hideColumn("dataRealizado");
  ui->tablePagamentos->hideColumn("valorReal");
  ui->tablePagamentos->hideColumn("tipoReal");
  ui->tablePagamentos->hideColumn("parcelaReal");
  ui->tablePagamentos->hideColumn("contaDestino");
  ui->tablePagamentos->hideColumn("tipoDet");
  ui->tablePagamentos->hideColumn("centroCusto");
  ui->tablePagamentos->hideColumn("grupo");
  ui->tablePagamentos->hideColumn("subGrupo");
  ui->tablePagamentos->hideColumn("taxa");
  ui->tablePagamentos->hideColumn("contraParte");
  ui->tablePagamentos->hideColumn("comissao");
  ui->tablePagamentos->hideColumn("desativado");

  ui->tablePagamentos->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePagamentos->setItemDelegateForColumn("representacao", new CheckBoxDelegate(this, true));

  ui->tablePagamentos->setPersistentColumns({"representacao"});

  //--------------------------------------------------------------

  modelVenda.setTable("venda");

  modelVenda.setFilter("idVenda = '" + idVenda + "'");

  if (not modelVenda.select()) { return; }

  //--------------------------------------------------------------

  modelCliente.setTable("cliente");

  modelCliente.setFilter("idCliente = " + modelVenda.data(0, "idCliente").toString());

  if (not modelCliente.select()) { return; }

  //--------------------------------------------------------------

  modelConsumos.setTable("estoque_has_consumo");

  //--------------------------------------------------------------

  modelCompra.setTable("pedido_fornecedor_has_produto2");
}

void Devolucao::on_tableProdutos_clicked(const QModelIndex &index) {
  if (not index.isValid()) {
    limparCampos();
    return;
  }

  const int row = index.row();

  const double quant = modelProdutos2.data(row, "quant").toDouble();
  const double caixas = modelProdutos2.data(row, "caixas").toDouble();
  const double quantCaixa = modelProdutos2.data(row, "quantCaixa").toDouble();
  const double total = modelProdutos2.data(row, "total").toDouble();
  const double prcUnitario = total / quant;
  const double credito = quant * prcUnitario;

  unsetConnections();

  ui->doubleSpinBoxPrecoUn->setValue(prcUnitario);

  ui->doubleSpinBoxCaixas->setMaximum(caixas);
  ui->doubleSpinBoxQuant->setMaximum(quant);
  ui->doubleSpinBoxCredito->setMaximum(credito);

  ui->doubleSpinBoxCaixas->setValue(caixas);
  ui->doubleSpinBoxQuant->setValue(quant);
  ui->doubleSpinBoxPorcentagem->setValue(100.);
  ui->doubleSpinBoxCredito->setValue(credito);

  ui->doubleSpinBoxQuant->setSingleStep(quantCaixa);
  ui->doubleSpinBoxQuant->setSuffix(" " + modelProdutos2.data(row, "un").toString());

  setConnections();
}

void Devolucao::on_doubleSpinBoxCaixas_valueChanged(const double caixas) {
  const double prcUnitario = ui->doubleSpinBoxPrecoUn->value();
  const double stepCaixas = ui->doubleSpinBoxCaixas->singleStep();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double caixasCorrigido = round(caixas / stepCaixas) * stepCaixas;
  const double quantCorrigido = caixasCorrigido * stepQuant;
  const double porcentagem = ui->doubleSpinBoxPorcentagem->value() / 100.;

  unsetConnections();

  ui->doubleSpinBoxCaixas->setValue(caixasCorrigido);
  ui->doubleSpinBoxQuant->setValue(quantCorrigido);
  ui->doubleSpinBoxCredito->setMaximum(quantCorrigido * prcUnitario);
  ui->doubleSpinBoxCredito->setValue(quantCorrigido * prcUnitario * porcentagem);

  setConnections();
}

void Devolucao::on_doubleSpinBoxQuant_valueChanged(const double quant) {
  const double prcUnitario = ui->doubleSpinBoxPrecoUn->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double quantCorrigido = round(quant / stepQuant) * stepQuant;
  const double caixasCorrigido = quantCorrigido / stepQuant;
  const double porcentagem = ui->doubleSpinBoxPorcentagem->value() / 100.;

  unsetConnections();

  ui->doubleSpinBoxCaixas->setValue(caixasCorrigido);
  ui->doubleSpinBoxQuant->setValue(quantCorrigido);
  ui->doubleSpinBoxCredito->setMaximum(quantCorrigido * prcUnitario);
  ui->doubleSpinBoxCredito->setValue(quantCorrigido * prcUnitario * porcentagem);

  setConnections();
}

void Devolucao::on_doubleSpinBoxPorcentagem_valueChanged(const double porcentagem) {
  const double prcUnitario = ui->doubleSpinBoxPrecoUn->value();
  const double quant = ui->doubleSpinBoxQuant->value();
  const double total = quant * prcUnitario;
  const double credito = total * (porcentagem / 100.);

  unsetConnections();

  ui->doubleSpinBoxCredito->setValue(credito);

  setConnections();
}

void Devolucao::on_doubleSpinBoxCredito_valueChanged(const double credito) {
  const double prcUnitario = ui->doubleSpinBoxPrecoUn->value();
  const double quant = ui->doubleSpinBoxQuant->value();
  const double total = quant * prcUnitario;
  const double porcentagem = (credito / total) * 100;

  unsetConnections();

  ui->doubleSpinBoxPorcentagem->setValue(porcentagem);

  setConnections();
}

bool Devolucao::criarDevolucao() {
  QSqlQuery query2;
  query2.prepare("SELECT COALESCE(RIGHT(MAX(IDVENDA), 1) + 1, 1) AS number FROM venda WHERE idVenda LIKE :idVenda");
  query2.bindValue(":idVenda", idVenda + "D%");

  if (not query2.exec() or not query2.first()) { return qApp->enqueueError(false, "Erro determinando próximo id: " + query2.lastError().text(), this); }

  idDevolucao = idVenda + "D" + query2.value("number").toString();

  // -------------------------------------------------------------------------

  const int newRow = modelVenda.insertRowAtEnd();

  for (int column = 0, columnCount = modelVenda.columnCount(); column < columnCount; ++column) {
    if (column == modelVenda.fieldIndex("idVendaBase")) { continue; }
    if (column == modelVenda.fieldIndex("created")) { continue; }
    if (column == modelVenda.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelVenda.data(0, column);

    if (not modelVenda.setData(newRow, column, value)) { return false; }
  }

  if (not modelVenda.setData(newRow, "idVenda", idDevolucao)) { return false; }
  if (not modelVenda.setData(newRow, "data", qApp->serverDateTime())) { return false; }
  if (not modelVenda.setData(newRow, "subTotalBru", 0)) { return false; }
  if (not modelVenda.setData(newRow, "subTotalLiq", 0)) { return false; }
  if (not modelVenda.setData(newRow, "descontoPorc", 0)) { return false; }
  if (not modelVenda.setData(newRow, "descontoReais", 0)) { return false; }
  if (not modelVenda.setData(newRow, "frete", 0)) { return false; }
  if (not modelVenda.setData(newRow, "total", 0)) { return false; }
  if (not modelVenda.setData(newRow, "prazoEntrega", 0)) { return false; }
  if (not modelVenda.setData(newRow, "devolucao", true)) { return false; }
  if (not modelVenda.setData(newRow, "status", "DEVOLVIDO")) { return false; }

  if (not modelVenda.submitAll()) { return false; }

  return true;
}

bool Devolucao::inserirItens(const int currentRow, const int novoIdVendaProduto2) {
  if (not copiarProdutoParaDevolucao(currentRow)) { return false; }

  if (not lerConsumos(currentRow)) { return false; }

  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();

  if (modelConsumos.rowCount() > 0) { // consumo referente a quant. devolvida
    if (not modelConsumos.setData(0, "quant", quantDevolvida * -1)) { return false; }
    if (not modelConsumos.setData(0, "caixas", (quantDevolvida / stepQuant))) { return false; }
  }

  //------------------------------------

  const double quant = modelProdutos2.data(currentRow, "quant").toDouble();
  const double restante = quant - quantDevolvida;

  if (restante > 0) {
    if (not dividirVenda(currentRow, novoIdVendaProduto2)) { return false; }
    if (not dividirCompra(currentRow, novoIdVendaProduto2)) { return false; }

    //------------------------------------

    if (modelConsumos.rowCount() > 0) {
      if (not dividirConsumo(currentRow, novoIdVendaProduto2)) { return false; }
    }
  }

  //------------------------------------

  if (not alterarLinhaOriginal(currentRow)) { return false; }

  //------------------------------------

  if (not modelProdutos2.submitAll()) { return false; }

  if (not modelCompra.submitAll()) { return false; }

  if (not modelConsumos.submitAll()) { return false; }

  return true;
}

bool Devolucao::criarContas() {
  // TODO: 0considerar a 'conta cliente' e ajustar as telas do financeiro para poder visualizar/trabalhar os dois fluxos
  // de contas

  if (qFuzzyIsNull(ui->doubleSpinBoxCredito->value())) { return true; }

  //--------------------------------------------------------------

  const int newRow = modelPagamentos.insertRowAtEnd();

  if (not modelPagamentos.setData(newRow, "contraParte", modelCliente.data(0, "nome_razao"))) { return false; }
  if (not modelPagamentos.setData(newRow, "dataEmissao", qApp->serverDate())) { return false; }
  if (not modelPagamentos.setData(newRow, "idVenda", idDevolucao)) { return false; }
  if (not modelPagamentos.setData(newRow, "idLoja", UserSession::idLoja())) { return false; }
  if (not modelPagamentos.setData(newRow, "valor", ui->doubleSpinBoxCredito->value() * -1)) { return false; }
  if (not modelPagamentos.setData(newRow, "tipo", "1. Conta Cliente")) { return false; }
  if (not modelPagamentos.setData(newRow, "parcela", 1)) { return false; }
  if (not modelPagamentos.setData(newRow, "observacao", "")) { return false; }
  if (not modelPagamentos.setData(newRow, "status", "RECEBIDO")) { return false; }
  if (not modelPagamentos.setData(newRow, "dataPagamento", qApp->serverDate())) { return false; }
  if (not modelPagamentos.setData(newRow, "dataRealizado", qApp->serverDate())) { return false; }
  if (not modelPagamentos.setData(newRow, "valorReal", ui->doubleSpinBoxCredito->value() * -1)) { return false; }

  const int contaCreditos = 11;
  if (not modelPagamentos.setData(newRow, "contaDestino", contaCreditos)) { return false; }

  if (not modelPagamentos.submitAll()) { return false; }

  return true;
}

bool Devolucao::salvarCredito() {
  if (qFuzzyIsNull(ui->doubleSpinBoxCredito->value())) { return true; }

  const double credito = modelCliente.data(0, "credito").toDouble() + ui->doubleSpinBoxCredito->value();

  if (not modelCliente.setData(0, "credito", credito)) { return false; }

  if (not modelCliente.submitAll()) { return false; }

  return true;
}

bool Devolucao::devolverItem(const int currentRow, const int novoIdVendaProduto2) {
  const QString idVenda = modelProdutos2.data(currentRow, "idVenda").toString();

  if (not determinarIdDevolucao()) { return false; }

  if (not criarContas()) { return false; }
  if (not salvarCredito()) { return false; }
  if (not inserirItens(currentRow, novoIdVendaProduto2)) { return false; }
  if (not atualizarDevolucao()) { return false; }

  if (not Sql::updateVendaStatus(idVenda)) { return false; }

  return true;
}

void Devolucao::limparCampos() {
  ui->doubleSpinBoxCaixas->clear();
  ui->doubleSpinBoxCredito->clear();
  ui->doubleSpinBoxPrecoUn->clear();
  ui->doubleSpinBoxQuant->setSuffix("");
  ui->doubleSpinBoxQuant->clear();
  ui->doubleSpinBoxPorcentagem->clear();

  ui->tableProdutos->clearSelection();
}

void Devolucao::on_pushButtonDevolverItem_clicked() {
  //  10cx

  //  1cx devolvido -> -1cx consumo
  //  9cx entregue  -> -9cx consumo
  //  D1 1cx pend. dev. -> sem consumo
  //  D1 1cx dev. forn. -> sem consumo
  //  D1 1cx dev. est.  -> +1cx

  //------------------------------------

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuant->value())) { return qApp->enqueueError("Não selecionou quantidade!", this); }

  // -------------------------------------------------------------------------

  const auto novoIdVendaProduto2 = qApp->reservarIdVendaProduto2();

  if (not novoIdVendaProduto2) { return; }

  // -------------------------------------------------------------------------

  if (not qApp->startTransaction("Devolucao::on_pushButtonDevolverItem")) { return; }

  if (not devolverItem(list.first().row(), novoIdVendaProduto2.value())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  // -------------------------------------------------------------------------

  limparCampos();

  qApp->enqueueInformation("Devolução realizada com sucesso!", this);
}

bool Devolucao::atualizarDevolucao() {
  QSqlQuery query;
  query.prepare("SELECT SUM(parcial) AS parcial, SUM(parcialDesc) AS parcialDesc FROM venda_has_produto2 WHERE idVenda = :idVenda");
  query.bindValue(":idVenda", idDevolucao);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dados da devolução: " + query.lastError().text(), this); }

  QSqlQuery query2;
  query2.prepare("UPDATE venda SET subTotalBru = :subTotalBru, subTotalLiq = :subTotalLiq, total = :total WHERE idVenda = :idVenda");
  query2.bindValue(":subTotalBru", query.value("parcial"));
  query2.bindValue(":subTotalLiq", query.value("parcialDesc"));
  query2.bindValue(":total", query.value("parcialDesc"));
  query2.bindValue(":idVenda", idDevolucao);

  if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando devolução: " + query2.lastError().text(), this); }

  return true;
}

bool Devolucao::copiarProdutoParaDevolucao(const int currentRow) {
  // copiar linha vp2 para vp1
  const int newRow = modelDevolvidos1.insertRowAtEnd();

  for (int column = 0; column < modelProdutos2.columnCount(); ++column) {
    if (column == modelProdutos2.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelProdutos2.fieldIndex("idVendaProdutoFK")) { continue; }
    if (column == modelProdutos2.fieldIndex("idNFeSaida")) { continue; }
    if (column == modelProdutos2.fieldIndex("created")) { continue; }
    if (column == modelProdutos2.fieldIndex("lastUpdated")) { continue; }

    const int columnIndex = modelDevolvidos1.fieldIndex(modelProdutos2.record().fieldName(column));

    if (columnIndex == -1) { continue; }

    const QVariant value = modelProdutos2.data(currentRow, column);

    if (not modelDevolvidos1.setData(newRow, columnIndex, value)) { return false; }
  }

  //--------------------------------------------------------------

  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double caixas = quantDevolvida / stepQuant * -1;
  const double quantDevolvidaInvertida = quantDevolvida * -1;
  const double prcUnitario = modelProdutos2.data(currentRow, "total").toDouble() / modelProdutos2.data(currentRow, "quant").toDouble() * -1;
  const double total = prcUnitario * quantDevolvida;

  if (not modelDevolvidos1.setData(newRow, "idVenda", idDevolucao)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "idRelacionado", modelProdutos2.data(currentRow, "idVendaProdutoFK"))) { return false; }
  if (not modelDevolvidos1.setData(newRow, "status", "PENDENTE DEV.")) { return false; }
  if (not modelDevolvidos1.setData(newRow, "statusOriginal", modelProdutos2.data(currentRow, "status"))) { return false; }
  if (not modelDevolvidos1.setData(newRow, "prcUnitario", prcUnitario)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "descUnitario", prcUnitario)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "caixas", caixas)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "quant", quantDevolvidaInvertida)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "parcial", total)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "desconto", 0)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "parcialDesc", total)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "descGlobal", 0)) { return false; }
  if (not modelDevolvidos1.setData(newRow, "total", total)) { return false; }

  if (not modelDevolvidos1.submitAll()) { return false; } // trigger copies into venda_has_produto2

  //--------------------------------------------------------------

  if (not atualizarIdRelacionado(currentRow)) { return false; }

  return true;
}

bool Devolucao::atualizarIdRelacionado(const int currentRow) {
  // get modelDevolvidos lastInsertId for modelConsumos
  const QString idVendaProduto1_Devolucao = modelDevolvidos1.query().lastInsertId().toString();

  QSqlQuery queryBusca;

  if (not queryBusca.exec("SELECT idVendaProduto2 FROM venda_has_produto2 WHERE idVendaProdutoFK = " + idVendaProduto1_Devolucao) or not queryBusca.first()) {
    return qApp->enqueueError(false, "Erro buscando idVendaProduto2: " + queryBusca.lastError().text(), this);
  }

  //------------------------------------

  QSqlQuery queryRelacionado;

  if (not queryRelacionado.exec("UPDATE venda_has_produto2 SET idRelacionado = " + modelProdutos2.data(currentRow, "idVendaProduto2").toString() +
                                " WHERE idVendaProduto2 = " + queryBusca.value("idVendaProduto2").toString())) {
    return qApp->enqueueError(false, "Erro marcando linha relacionada: " + queryRelacionado.lastError().text(), this);
  }

  return true;
}

bool Devolucao::lerConsumos(const int currentRow) {
  modelConsumos.setFilter("idVendaProduto2 = " + modelProdutos2.data(currentRow, "idVendaProduto2").toString());

  if (not modelConsumos.select()) { return qApp->enqueueError(false, "Erro lendo consumos: " + modelConsumos.lastError().text(), this); }

  return true;
}

bool Devolucao::dividirVenda(const int currentRow, const int novoIdVendaProduto2) {
  // NOTE: *quebralinha venda_produto2
  const int newRow = modelProdutos2.insertRowAtEnd();

  for (int column = 0; column < modelProdutos2.columnCount(); ++column) {
    if (column == modelProdutos2.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelProdutos2.fieldIndex("created")) { continue; }
    if (column == modelProdutos2.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelProdutos2.data(currentRow, column);

    if (not modelProdutos2.setData(newRow, column, value)) { return false; }
  }

  //--------------------------------------------------------------------

  const double quant = modelProdutos2.data(currentRow, "quant").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double restante = quant - quantDevolvida;
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double prcUnitario = modelProdutos2.data(newRow, "prcUnitario").toDouble();
  const double parcial = restante * prcUnitario;
  const double desconto = modelProdutos2.data(newRow, "desconto").toDouble();
  const double parcialDesc = parcial * (1 - (desconto / 100));
  const double descGlobal = modelProdutos2.data(newRow, "descGlobal").toDouble();
  const double total = parcialDesc * (1 - (descGlobal / 100));

  if (not modelProdutos2.setData(newRow, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
  if (not modelProdutos2.setData(newRow, "idRelacionado", modelProdutos2.data(currentRow, "idVendaProduto2"))) { return false; }
  if (not modelProdutos2.setData(newRow, "caixas", (restante / stepQuant))) { return false; }
  if (not modelProdutos2.setData(newRow, "quant", restante)) { return false; }
  if (not modelProdutos2.setData(newRow, "parcial", parcial)) { return false; }
  if (not modelProdutos2.setData(newRow, "parcialDesc", parcialDesc)) { return false; }
  if (not modelProdutos2.setData(newRow, "total", total)) { return false; }

  return true;
}

bool Devolucao::dividirCompra(const int currentRow, const int novoIdVendaProduto2) {
  modelCompra.setFilter("idVendaProduto2 = " + modelProdutos2.data(currentRow, "idVendaProduto2").toString());

  if (not modelCompra.select()) { return false; }

  if (modelCompra.rowCount() == 0) { return true; }

  //--------------------------------------------------------------------

  const double quantOriginal = modelCompra.data(0, "quant").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double prcUnitario = modelCompra.data(0, "prcUnitario").toDouble();
  const double quantRestante = quantOriginal - quantDevolvida;
  const QString status = modelCompra.data(0, "status").toString();

  if (not modelCompra.setData(0, "status", "DEVOLVIDO")) { return false; }
  if (not modelCompra.setData(0, "quant", quantDevolvida)) { return false; }
  if (not modelCompra.setData(0, "caixas", quantDevolvida / stepQuant)) { return false; }
  if (not modelCompra.setData(0, "preco", prcUnitario * quantDevolvida)) { return false; }

  //--------------------------------------------------------------------

  // NOTE: *quebralinha pedido_fornecedor2
  const int newRow = modelCompra.insertRowAtEnd();

  for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
    if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
    if (column == modelCompra.fieldIndex("created")) { continue; }
    if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelCompra.data(0, column);

    if (not modelCompra.setData(newRow, column, value)) { return false; }
  }

  //--------------------------------------------------------------------

  if (not modelCompra.setData(newRow, "idRelacionado", modelCompra.data(0, "idPedido2"))) { return false; }
  if (not modelCompra.setData(newRow, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
  if (not modelCompra.setData(newRow, "status", status)) { return false; }
  if (not modelCompra.setData(newRow, "quant", quantRestante)) { return false; }
  if (not modelCompra.setData(newRow, "caixas", quantRestante / stepQuant)) { return false; }
  if (not modelCompra.setData(newRow, "preco", prcUnitario * quantRestante)) { return false; }

  //--------------------------------------------------------------------

  return true;
}

bool Devolucao::dividirConsumo(const int currentRow, const int novoIdVendaProduto2) {
  const int newRow = modelConsumos.insertRowAtEnd();

  for (int column = 0; column < modelConsumos.columnCount(); ++column) {
    if (column == modelConsumos.fieldIndex("idConsumo")) { continue; }
    if (column == modelConsumos.fieldIndex("created")) { continue; }
    if (column == modelConsumos.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelConsumos.data(0, column);

    if (not modelConsumos.setData(newRow, column, value)) { return false; }
  }

  //--------------------------------------------------------------------

  const double quant = modelProdutos2.data(currentRow, "quant").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double restante = quant - quantDevolvida;
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();

  if (not modelConsumos.setData(newRow, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
  if (not modelConsumos.setData(newRow, "quant", restante * -1)) { return false; }
  if (not modelConsumos.setData(newRow, "caixas", (restante / stepQuant))) { return false; }

  return true;
}

bool Devolucao::alterarLinhaOriginal(const int currentRow) {
  const double prcUnitario = modelProdutos2.data(currentRow, "prcUnitario").toDouble();
  const double desconto = modelProdutos2.data(currentRow, "desconto").toDouble();
  const double descGlobal = modelProdutos2.data(currentRow, "descGlobal").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double caixas = quantDevolvida / stepQuant;
  const double parcialRestante = quantDevolvida * prcUnitario;
  const double parcialDescRestante = parcialRestante * (1 - (desconto / 100));
  const double totalRestante = parcialDescRestante * (1 - (descGlobal / 100));

  if (not modelProdutos2.setData(currentRow, "caixas", caixas)) { return false; }
  if (not modelProdutos2.setData(currentRow, "quant", quantDevolvida)) { return false; }
  if (not modelProdutos2.setData(currentRow, "parcial", parcialRestante)) { return false; }
  if (not modelProdutos2.setData(currentRow, "parcialDesc", parcialDescRestante)) { return false; }
  if (not modelProdutos2.setData(currentRow, "total", totalRestante)) { return false; }
  if (not modelProdutos2.setData(currentRow, "statusOriginal", modelProdutos2.data(currentRow, "status"))) { return false; }
  if (not modelProdutos2.setData(currentRow, "status", "DEVOLVIDO")) { return false; }

  return true;
}

// TODO: 0. lidar com os casos em que o produto estava agendado e é feita a devolucao, alterando os consumos
// TODO: 1. perguntar e guardar data em que ocorreu a devolucao
// TODO: 2. ??? nao criar linha conta
// TODO: 2. adicionar devolucao de frete quando houver
// TODO: 2. criar linha no followup
// TODO: 2. quando for devolver para o fornecedor perguntar a quantidade
