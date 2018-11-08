#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "QDecDouble.hh"
#include "application.h"
#include "checkboxdelegate.h"
#include "devolucao.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "ui_devolucao.h"
#include "usersession.h"

Devolucao::Devolucao(QString idVenda, QWidget *parent) : QDialog(parent), idVenda(std::move(idVenda)), ui(new Ui::Devolucao) {
  ui->setupUi(this);

  connect(ui->doubleSpinBoxCaixas, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCaixas_valueChanged);
  connect(ui->doubleSpinBoxQuant, &QDoubleSpinBox::editingFinished, this, &Devolucao::on_doubleSpinBoxQuant_editingFinished);
  connect(ui->doubleSpinBoxQuant, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxQuant_valueChanged);
  connect(ui->doubleSpinBoxTotalItem, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxTotalItem_valueChanged);
  connect(ui->pushButtonDevolverItem, &QPushButton::clicked, this, &Devolucao::on_pushButtonDevolverItem_clicked);
  connect(ui->tableProdutos, &TableView::clicked, this, &Devolucao::on_tableProdutos_clicked);

  setWindowFlags(Qt::Window);

  determinarIdDevolucao();

  setupTables();
}

Devolucao::~Devolucao() { delete ui; }

void Devolucao::determinarIdDevolucao() {
  QSqlQuery query;
  query.prepare("SELECT MAX(idVenda) AS id FROM venda WHERE idVenda LIKE :idVenda AND MONTH(data) = :month HAVING MAX(idVenda) IS NOT NULL");
  query.bindValue(":idVenda", idVenda + "D%");
  query.bindValue(":month", QDate::currentDate().month());

  if (not query.exec()) { return qApp->enqueueError("Erro verificando se existe devolução: " + query.lastError().text()); }

  if (query.first()) {
    idDevolucao = query.value("id").toString();
  } else {
    QSqlQuery query2;
    query2.prepare("SELECT COALESCE(RIGHT(MAX(IDVENDA), 1) + 1, 1) AS number FROM venda WHERE idVenda LIKE :idVenda");
    query2.bindValue(":idVenda", idVenda + "D%");

    if (not query2.exec() or not query2.first()) { qApp->enqueueError("Erro determinando próximo id: " + query2.lastError().text()); }

    idDevolucao = idVenda + "D" + query2.value("number").toString();
    createNewId = true;
  }
}

void Devolucao::setupTables() {
  modelProdutos.setTable("venda_has_produto");
  modelProdutos.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelProdutos.setHeaderData("status", "Status");
  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("produto", "Produto");
  modelProdutos.setHeaderData("obs", "Obs.");
  modelProdutos.setHeaderData("prcUnitario", "R$ Unit.");
  modelProdutos.setHeaderData("caixas", "Caixas");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("unCaixa", "Un. Caixa");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");
  modelProdutos.setHeaderData("total", "Total");

  modelProdutos.setFilter("idVenda = '" + idVenda + "' AND status != 'DEVOLVIDO'");

  if (not modelProdutos.select()) { return; }

  ui->tableProdutos->setModel(&modelProdutos);
  ui->tableProdutos->hideColumn("idRelacionado");
  ui->tableProdutos->hideColumn("statusOriginal");
  ui->tableProdutos->hideColumn("mostrarDesconto");
  ui->tableProdutos->hideColumn("reposicaoEntrega");
  ui->tableProdutos->hideColumn("reposicaoReceb");
  ui->tableProdutos->hideColumn("recebeu");
  ui->tableProdutos->hideColumn("entregou");
  ui->tableProdutos->hideColumn("descUnitario");
  ui->tableProdutos->hideColumn("parcial");
  ui->tableProdutos->hideColumn("desconto");
  ui->tableProdutos->hideColumn("parcialDesc");
  ui->tableProdutos->hideColumn("descGlobal");
  ui->tableProdutos->hideColumn("descGlobal");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("selecionado");
  ui->tableProdutos->hideColumn("idVendaProduto");
  ui->tableProdutos->hideColumn("idNFeSaida");
  ui->tableProdutos->hideColumn("idNFeFutura");
  ui->tableProdutos->hideColumn("idLoja");
  ui->tableProdutos->hideColumn("idVenda");
  ui->tableProdutos->hideColumn("item");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idCompra");
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
  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("total", new ReaisDelegate(this));

  ui->tableProdutos->resizeColumnsToContents();

  modelDevolvidos.setTable("venda_has_produto");
  modelDevolvidos.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelDevolvidos.setHeaderData("selecionado", "");
  modelDevolvidos.setHeaderData("status", "Status");
  modelDevolvidos.setHeaderData("statusOriginal", "Status Original");
  modelDevolvidos.setHeaderData("fornecedor", "Fornecedor");
  modelDevolvidos.setHeaderData("produto", "Produto");
  modelDevolvidos.setHeaderData("obs", "Obs.");
  modelDevolvidos.setHeaderData("prcUnitario", "R$ Unit.");
  modelDevolvidos.setHeaderData("caixas", "Caixas");
  modelDevolvidos.setHeaderData("quant", "Quant.");
  modelDevolvidos.setHeaderData("un", "Un.");
  modelDevolvidos.setHeaderData("unCaixa", "Un. Caixa");
  modelDevolvidos.setHeaderData("codComercial", "Cód. Com.");
  modelDevolvidos.setHeaderData("formComercial", "Form. Com.");
  modelDevolvidos.setHeaderData("total", "Total");

  modelDevolvidos.setFilter("idVenda = '" + idDevolucao + "'");

  if (not modelDevolvidos.select()) { return; }

  ui->tableDevolvidos->setModel(&modelDevolvidos);
  ui->tableDevolvidos->hideColumn("idRelacionado");
  ui->tableDevolvidos->hideColumn("mostrarDesconto");
  ui->tableDevolvidos->hideColumn("reposicaoEntrega");
  ui->tableDevolvidos->hideColumn("reposicaoReceb");
  ui->tableDevolvidos->hideColumn("recebeu");
  ui->tableDevolvidos->hideColumn("entregou");
  ui->tableDevolvidos->hideColumn("descUnitario");
  ui->tableDevolvidos->hideColumn("selecionado");
  ui->tableDevolvidos->hideColumn("idVendaProduto");
  ui->tableDevolvidos->hideColumn("idNFeSaida");
  ui->tableDevolvidos->hideColumn("idNFeFutura");
  ui->tableDevolvidos->hideColumn("idLoja");
  ui->tableDevolvidos->hideColumn("idVenda");
  ui->tableDevolvidos->hideColumn("item");
  ui->tableDevolvidos->hideColumn("idProduto");
  ui->tableDevolvidos->hideColumn("idCompra");
  ui->tableDevolvidos->hideColumn("parcial");
  ui->tableDevolvidos->hideColumn("desconto");
  ui->tableDevolvidos->hideColumn("parcialDesc");
  ui->tableDevolvidos->hideColumn("descGlobal");
  ui->tableDevolvidos->hideColumn("estoque");
  ui->tableDevolvidos->hideColumn("promocao");
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
  ui->tableDevolvidos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableDevolvidos->setItemDelegateForColumn("total", new ReaisDelegate(this));

  modelPagamentos.setTable("conta_a_receber_has_pagamento");
  modelPagamentos.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelPagamentos.setHeaderData("tipo", "Tipo");
  modelPagamentos.setHeaderData("parcela", "Parcela");
  modelPagamentos.setHeaderData("valor", "R$");
  modelPagamentos.setHeaderData("dataPagamento", "Data");
  modelPagamentos.setHeaderData("observacao", "Obs.");
  modelPagamentos.setHeaderData("status", "Status");
  modelPagamentos.setHeaderData("representacao", "Representação");

  modelPagamentos.setFilter("idVenda = '" + idDevolucao + "'");

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
  ui->tablePagamentos->setItemDelegateForColumn(modelPagamentos.fieldIndex("valor"), new ReaisDelegate(this));
  ui->tablePagamentos->setItemDelegateForColumn(modelPagamentos.fieldIndex("representacao"), new CheckBoxDelegate(this, true));

  for (int row = 0; row < modelPagamentos.rowCount(); ++row) { ui->tablePagamentos->openPersistentEditor(row, "representacao"); }

  ui->tablePagamentos->resizeColumnsToContents();

  modelVenda.setTable("venda");
  modelVenda.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelVenda.setFilter("idVenda = '" + idVenda + "'");

  if (not modelVenda.select()) { return; }

  modelCliente.setTable("cliente");
  modelCliente.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelCliente.setFilter("idCliente = " + modelVenda.data(0, "idCliente").toString());

  if (not modelCliente.select()) { return; }

  // mapper
  mapperItem.setModel(&modelProdutos);
  mapperItem.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

  mapperItem.addMapping(ui->doubleSpinBoxQuant, modelProdutos.fieldIndex("quant"));
  mapperItem.addMapping(ui->doubleSpinBoxCaixas, modelProdutos.fieldIndex("caixas"));
  mapperItem.addMapping(ui->lineEditUn, modelProdutos.fieldIndex("un"));
}

void Devolucao::on_tableProdutos_clicked(const QModelIndex &index) {
  const double total = modelProdutos.data(index.row(), "total").toDouble();

  const auto list = modelDevolvidos.match("idRelacionado", modelProdutos.data(index.row(), "idVendaProduto"), -1, Qt::MatchExactly);

  double quantDevolvida = 0;
  double caixasDevolvidas = 0;

  for (const auto &index2 : list) {
    quantDevolvida += modelDevolvidos.data(index2.row(), "quant").toDouble();
    caixasDevolvidas += modelDevolvidos.data(index2.row(), "caixas").toDouble();
  }

  const double quant = modelProdutos.data(index.row(), "quant").toDouble() + quantDevolvida;
  const double caixas = modelProdutos.data(index.row(), "caixas").toDouble() + caixasDevolvidas;

  ui->doubleSpinBoxQuant->setSingleStep(quant / caixas);

  ui->doubleSpinBoxQuant->setMaximum(quant);
  ui->doubleSpinBoxCaixas->setMaximum(caixas);

  mapperItem.setCurrentModelIndex(index);

  ui->doubleSpinBoxPrecoUn->setValue(total / quant);

  calcPrecoItemTotal();
}

void Devolucao::calcPrecoItemTotal() { ui->doubleSpinBoxTotalItem->setValue(ui->doubleSpinBoxQuant->value() * ui->doubleSpinBoxPrecoUn->value()); }

void Devolucao::on_doubleSpinBoxCaixas_valueChanged(const double caixas) {
  const double quant = caixas * ui->doubleSpinBoxQuant->singleStep();

  if (not qFuzzyCompare(ui->doubleSpinBoxQuant->value(), quant)) { ui->doubleSpinBoxQuant->setValue(quant); }

  calcPrecoItemTotal();
}

void Devolucao::on_doubleSpinBoxQuant_valueChanged(double) {
  const double caixas = qRound(ui->doubleSpinBoxQuant->value() / ui->doubleSpinBoxQuant->singleStep() * 100) / 100.;

  if (not qFuzzyCompare(ui->doubleSpinBoxCaixas->value(), caixas)) { ui->doubleSpinBoxCaixas->setValue(caixas); }
}

void Devolucao::on_doubleSpinBoxQuant_editingFinished() { ui->doubleSpinBoxQuant->setValue(ui->doubleSpinBoxCaixas->value() * ui->doubleSpinBoxQuant->singleStep()); }

bool Devolucao::criarDevolucao() {
  const int newRow = modelVenda.rowCount();
  if (not modelVenda.insertRow(newRow)) { return false; }

  for (int column = 0, columnCount = modelVenda.columnCount(); column < columnCount; ++column) {
    if (modelVenda.fieldIndex("idVendaBase") == column) { continue; }
    if (modelVenda.fieldIndex("created") == column) { continue; }
    if (modelVenda.fieldIndex("lastUpdated") == column) { continue; }

    const QVariant value = modelVenda.data(0, column);

    if (not modelVenda.setData(newRow, column, value)) { return false; }
  }

  if (not modelVenda.setData(newRow, "idVenda", idDevolucao)) { return false; }
  if (not modelVenda.setData(newRow, "data", QDateTime::currentDateTime())) { return false; }
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

bool Devolucao::inserirItens(const int currentRow) {
  const QDecDouble quant = modelProdutos.data(mapperItem.currentIndex(), "quant").toDouble();
  const QDecDouble quantDevolvida = ui->doubleSpinBoxQuant->value();
  const QDecDouble restante = quant - quantDevolvida;
  const QDecDouble step = ui->doubleSpinBoxQuant->singleStep();

  // copiar linha para devolucao
  const int rowDevolucao = modelDevolvidos.rowCount();
  if (not modelDevolvidos.insertRow(rowDevolucao)) { return false; }

  for (int column = 0; column < modelProdutos.columnCount(); ++column) {
    if (modelProdutos.fieldIndex("idVendaProduto") == column) { continue; }
    if (modelProdutos.fieldIndex("created") == column) { continue; }
    if (modelProdutos.fieldIndex("lastUpdated") == column) { continue; }
    if (modelProdutos.fieldIndex("idNFeSaida") == column) { continue; }

    const QVariant value = modelProdutos.data(currentRow, column);

    if (not modelDevolvidos.setData(rowDevolucao, column, value)) { return false; }
  }

  if (not modelDevolvidos.setData(rowDevolucao, "idVenda", idDevolucao)) { return false; }
  const QDecDouble quantDevolvidaInvertida = quantDevolvida * -1;
  const QDecDouble parcialDevolvido = ui->doubleSpinBoxTotalItem->value() * -1;
  const QDecDouble prcUnitarioDevolvido = parcialDevolvido / quantDevolvidaInvertida;
  if (not modelDevolvidos.setData(rowDevolucao, "status", "PENDENTE DEV.")) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "statusOriginal", modelProdutos.data(currentRow, "status"))) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "prcUnitario", prcUnitarioDevolvido.toDouble())) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "descUnitario", prcUnitarioDevolvido.toDouble())) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "caixas", (quantDevolvida / step * -1).toDouble())) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "quant", quantDevolvidaInvertida.toDouble())) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "parcial", parcialDevolvido.toDouble())) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "desconto", 0)) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "parcialDesc", ui->doubleSpinBoxTotalItem->value() * -1)) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "descGlobal", 0)) { return false; }
  if (not modelDevolvidos.setData(rowDevolucao, "total", ui->doubleSpinBoxTotalItem->value() * -1)) { return false; }

  if (not modelDevolvidos.submitAll()) { return false; }

  //------------------------------------

  if (restante > 0) {
    const int newRowRestante = modelProdutos.rowCount();
    // NOTE: *quebralinha venda_produto/pedido_fornecedor
    if (not modelProdutos.insertRow(newRowRestante)) { return false; }

    for (int column = 0; column < modelProdutos.columnCount(); ++column) {
      if (modelProdutos.fieldIndex("idVendaProduto") == column) { continue; }
      if (modelProdutos.fieldIndex("created") == column) { continue; }
      if (modelProdutos.fieldIndex("lastUpdated") == column) { continue; }

      const QVariant value = modelProdutos.data(currentRow, column);

      if (not modelProdutos.setData(newRowRestante, column, value)) { return false; }
    }

    if (not modelProdutos.setData(newRowRestante, "caixas", (restante / step).toDouble())) { return false; }
    if (not modelProdutos.setData(newRowRestante, "quant", restante.toDouble())) { return false; }

    const QDecDouble prcUnitario = modelProdutos.data(newRowRestante, "prcUnitario").toDouble();
    const QDecDouble parcial = restante * prcUnitario;

    if (not modelProdutos.setData(newRowRestante, "parcial", parcial.toDouble())) { return false; }

    const QDecDouble desconto = modelProdutos.data(newRowRestante, "desconto").toDouble();
    const QDecDouble parcialDesc = parcial * (1 - (desconto / 100).toDouble());

    if (not modelProdutos.setData(newRowRestante, "parcialDesc", parcialDesc.toDouble())) { return false; }

    const QDecDouble descGlobal = modelProdutos.data(newRowRestante, "descGlobal").toDouble();
    const QDecDouble total = parcialDesc * (1 - (descGlobal / 100).toDouble());

    if (not modelProdutos.setData(newRowRestante, "total", total.toDouble())) { return false; }
  }

  // ------------------------------------

  const QDecDouble prcUnitario = modelProdutos.data(currentRow, "prcUnitario").toDouble();
  const QDecDouble desconto = modelProdutos.data(currentRow, "desconto").toDouble();
  const QDecDouble descGlobal = modelProdutos.data(currentRow, "descGlobal").toDouble();

  // linha original

  const QDecDouble caixas = quantDevolvida / step;

  if (not modelProdutos.setData(currentRow, "caixas", caixas.toDouble())) { return false; }
  if (not modelProdutos.setData(currentRow, "quant", quantDevolvida.toDouble())) { return false; }

  const QDecDouble parcialRestante = quantDevolvida * prcUnitario;

  if (not modelProdutos.setData(currentRow, "parcial", parcialRestante.toDouble())) { return false; }

  const QDecDouble parcialDescRestante = parcialRestante * (1 - (desconto / 100).toDouble());

  if (not modelProdutos.setData(currentRow, "parcialDesc", parcialDescRestante.toDouble())) { return false; }

  const QDecDouble totalRestante = parcialDescRestante * (1 - (descGlobal / 100).toDouble());

  if (not modelProdutos.setData(currentRow, "total", totalRestante.toDouble())) { return false; }
  if (not modelProdutos.setData(currentRow, "status", "DEVOLVIDO")) { return false; }

  //----------------------------------------------

  if (not modelProdutos.submitAll()) { return false; }

  return true;
}

bool Devolucao::criarContas() {
  // TODO: 0considerar a 'conta cliente' e ajustar as telas do financeiro para poder visualizar/trabalhar os dois fluxos
  // de contas

  const int newRowPag = modelPagamentos.rowCount();
  if (not modelPagamentos.insertRow(newRowPag)) { return false; }

  if (not modelPagamentos.setData(newRowPag, "contraParte", modelCliente.data(0, "nome_razao"))) { return false; }
  if (not modelPagamentos.setData(newRowPag, "dataEmissao", QDate::currentDate())) { return false; }
  if (not modelPagamentos.setData(newRowPag, "idVenda", idDevolucao)) { return false; }
  if (not modelPagamentos.setData(newRowPag, "idLoja", UserSession::idLoja())) { return false; }
  if (not modelPagamentos.setData(newRowPag, "valor", ui->doubleSpinBoxCredito->value() * -1)) { return false; }
  if (not modelPagamentos.setData(newRowPag, "tipo", "1. Conta Cliente")) { return false; }
  if (not modelPagamentos.setData(newRowPag, "parcela", 1)) { return false; }
  if (not modelPagamentos.setData(newRowPag, "observacao", "")) { return false; }
  if (not modelPagamentos.setData(newRowPag, "status", "RECEBIDO")) { return false; }
  if (not modelPagamentos.setData(newRowPag, "dataPagamento", QDate::currentDate())) { return false; }
  if (not modelPagamentos.setData(newRowPag, "dataRealizado", QDate::currentDate())) { return false; }
  if (not modelPagamentos.setData(newRowPag, "valorReal", ui->doubleSpinBoxCredito->value() * -1)) { return false; }
  // REFAC: 0dont hardcode
  if (not modelPagamentos.setData(newRowPag, "contaDestino", 11)) { return false; }

  //----------------

  // TODO: verificar quando devo criar comissao negativa (quando gera credito ou sempre?)

  //  if (modelVenda.data(0, "representacao").toBool()) {
  //    const int row = modelPagamentos.rowCount();
  //    if (not modelPagamentos.insertRow(row)) { return false; }

  //    const QString fornecedor = modelProdutos.data(0, "fornecedor").toString();

  // TODO: 0no lugar de pegar a 'comissaoLoja' pegar a comissao do fluxo e calcular o proporcional

  //    QSqlQuery query;
  //    query.prepare("SELECT comissaoLoja FROM fornecedor WHERE razaoSocial = :razaoSocial");
  //    query.bindValue(":razaoSocial", fornecedor);

  //  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando comissão: " + query.lastError().text()); }

  //    const double comissao = query.value("comissaoLoja").toDouble() / 100;

  //    const double valor = ui->doubleSpinBoxCredito->value() * -1;

  //    double valorAjustado =
  //        comissao * (valor - (valor / modelVenda.data(0, "total").toDouble() * modelVenda.data(0,
  //        "frete").toDouble()));

  //    modelPagamentos.setData(row, "contraParte", fornecedor);
  //    modelPagamentos.setData(row, "dataEmissao", QDate::currentDate());
  //    modelPagamentos.setData(row, "idVenda", idDevolucao);
  //    modelPagamentos.setData(row, "idLoja", modelVenda.data(0, "idLoja"));
  // TODO: 0colocar mesma data do fluxo original
  //    modelPagamentos.setData(row, "dataPagamento", QDate::currentDate());
  //    modelPagamentos.setData(row, "valor", valorAjustado);
  //    modelPagamentos.setData(row, "tipo", "1. Comissão");
  //    modelPagamentos.setData(row, "parcela", 1);
  //    modelPagamentos.setData(row, "comissao", 1);
  //  }

  // -------------------------------------------------------------------------

  if (not modelPagamentos.submitAll()) { return false; }

  return true;
}

bool Devolucao::salvarCredito() {
  const double credito = modelCliente.data(0, "credito").toDouble() + ui->doubleSpinBoxCredito->value();

  if (not modelCliente.setData(0, "credito", credito)) { return false; }

  if (not modelCliente.submitAll()) { return false; }

  return true;
}

bool Devolucao::devolverItem(const int currentRow) {
  if (createNewId and not criarDevolucao()) { return false; }

  // REFAC: move submitAll outside those functions?

  if (not criarContas()) { return false; }
  if (not salvarCredito()) { return false; }
  if (not inserirItens(currentRow)) { return false; }
  if (not atualizarDevolucao()) { return false; }

  ui->tableProdutos->resizeColumnsToContents();
  ui->tableDevolvidos->resizeColumnsToContents();
  ui->tablePagamentos->resizeColumnsToContents();

  return true;
}

void Devolucao::limparCampos() {
  ui->doubleSpinBoxCaixas->clear();
  ui->doubleSpinBoxCredito->clear();
  ui->doubleSpinBoxPrecoUn->clear();
  ui->doubleSpinBoxQuant->clear();
  ui->doubleSpinBoxTotalItem->clear();
  ui->lineEditUn->clear();

  ui->tableProdutos->clearSelection();
}

void Devolucao::on_pushButtonDevolverItem_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!"); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuant->value())) { return qApp->enqueueError("Não selecionou quantidade!"); }

  if (not qApp->startTransaction()) { return; }

  if (not devolverItem(list.first().row())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  limparCampos();

  qApp->enqueueInformation("Devolução realizada com sucesso!");
}

bool Devolucao::atualizarDevolucao() {
  QSqlQuery query;
  query.prepare("SELECT SUM(parcial) AS parcial, SUM(parcialDesc) AS parcialDesc FROM venda_has_produto WHERE idVenda = :idVenda");
  query.bindValue(":idVenda", idDevolucao);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dados da devolução: " + query.lastError().text()); }

  QSqlQuery query2;
  query2.prepare("UPDATE venda SET subTotalBru = :subTotalBru, subTotalLiq = :subTotalLiq, total = :total WHERE idVenda = :idVenda");
  query2.bindValue(":subTotalBru", query.value("parcial"));
  query2.bindValue(":subTotalLiq", query.value("parcialDesc"));
  query2.bindValue(":total", query.value("parcialDesc"));
  query2.bindValue(":idVenda", idDevolucao);

  if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando devolução: " + query2.lastError().text()); }

  return true;
}

void Devolucao::on_doubleSpinBoxTotalItem_valueChanged(double value) {
  ui->doubleSpinBoxCredito->setMaximum(value);
  ui->doubleSpinBoxCredito->setValue(value);
}

// TODO: 2nao criar linha conta
// TODO: 2criar linha no followup
// TODO: 0verificar erro quando a devolucao do item é feita com valor menor, conta_cliente saiu certo mas venda_devolucao saiu com valor cheio
// TODO: 1perguntar e guardar data em que ocorreu a devolucao
// TODO: 2quando for devolver para o fornecedor perguntar a quantidade
// TODO: 2quando fizer devolucao no consumo/estoque alterar os dados no pedido_fornecedor? se as quantidades forem iguais trocar idVenda/idVendaProduto
// TODO: testar devolver mais de uma linha
// TODO: ao quebrar linha venda_has_produto em 2 ajustar os consumos de estoque para que fique devolvido<->devolvido e consumo<->consumo
// em vez de ficar os 2 consumos apontando para a mesma linha (select * from view_estoque_consumo where status = 'CONSUMO' and statusProduto = 'DEVOLVIDO';)
