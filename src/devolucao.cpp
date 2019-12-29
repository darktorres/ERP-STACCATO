#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "application.h"
#include "checkboxdelegate.h"
#include "devolucao.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "ui_devolucao.h"
#include "usersession.h"

Devolucao::Devolucao(const QString &idVenda, QWidget *parent) : QDialog(parent), idVenda(idVenda), ui(new Ui::Devolucao) {
  ui->setupUi(this);

  connect(ui->doubleSpinBoxCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCaixas_valueChanged);
  connect(ui->doubleSpinBoxQuant, &QDoubleSpinBox::editingFinished, this, &Devolucao::on_doubleSpinBoxQuant_editingFinished);
  connect(ui->doubleSpinBoxQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxQuant_valueChanged);
  connect(ui->doubleSpinBoxTotalItem, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxTotalItem_valueChanged);
  connect(ui->pushButtonDevolverItem, &QPushButton::clicked, this, &Devolucao::on_pushButtonDevolverItem_clicked);
  connect(ui->tableProdutos, &TableView::clicked, this, &Devolucao::on_tableProdutos_clicked);

  setWindowFlags(Qt::Window);

  setupTables();
}

Devolucao::~Devolucao() { delete ui; }

std::optional<bool> Devolucao::determinarIdDevolucao() {
  QSqlQuery query;
  query.prepare("SELECT MAX(idVenda) AS id FROM venda WHERE idVenda LIKE :idVenda AND MONTH(data) = MONTH(NOW()) HAVING MAX(idVenda) IS NOT NULL");
  query.bindValue(":idVenda", idVenda + "D%");

  if (not query.exec()) {
    qApp->enqueueError("Erro verificando se existe devolução: " + query.lastError().text(), this);
    return {};
  }

  bool createNewId;

  if (query.first()) {
    idDevolucao = query.value("id").toString();
    createNewId = false;
  } else {
    QSqlQuery query2;
    query2.prepare("SELECT COALESCE(RIGHT(MAX(IDVENDA), 1) + 1, 1) AS number FROM venda WHERE idVenda LIKE :idVenda");
    query2.bindValue(":idVenda", idVenda + "D%");

    if (not query2.exec() or not query2.first()) {
      qApp->enqueueError("Erro determinando próximo id: " + query2.lastError().text(), this);
      return {};
    }

    idDevolucao = idVenda + "D" + query2.value("number").toString();
    createNewId = true;
  }

  return createNewId;
}

void Devolucao::setupTables() {
  modelProdutos2.setTable("venda_has_produto2");

  modelProdutos2.setHeaderData("status", "Status");
  modelProdutos2.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos2.setHeaderData("produto", "Produto");
  modelProdutos2.setHeaderData("obs", "Obs.");
  modelProdutos2.setHeaderData("lote", "Lote");
  modelProdutos2.setHeaderData("prcUnitario", "R$ Unit.");
  modelProdutos2.setHeaderData("caixas", "Caixas");
  modelProdutos2.setHeaderData("quant", "Quant.");
  modelProdutos2.setHeaderData("un", "Un.");
  modelProdutos2.setHeaderData("unCaixa", "Un. Caixa");
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
  modelDevolvidos1.setHeaderData("unCaixa", "Un. Caixa");
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

  mapperProdutos.setModel(&modelProdutos2);
  mapperProdutos.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);

  mapperProdutos.addMapping(ui->doubleSpinBoxQuant, modelProdutos2.fieldIndex("quant"));
  mapperProdutos.addMapping(ui->doubleSpinBoxCaixas, modelProdutos2.fieldIndex("caixas"));
  mapperProdutos.addMapping(ui->lineEditUn, modelProdutos2.fieldIndex("un"));
}

void Devolucao::on_tableProdutos_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const int row = index.row();

  const double quant = modelProdutos2.data(row, "quant").toDouble();
  const double caixas = modelProdutos2.data(row, "caixas").toDouble();

  ui->doubleSpinBoxQuant->setSingleStep(quant / caixas);

  mapperProdutos.setCurrentModelIndex(index);

  const double total = modelProdutos2.data(row, "total").toDouble();

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
  const int newRow = modelVenda.insertRowAtEnd();

  for (int column = 0, columnCount = modelVenda.columnCount(); column < columnCount; ++column) {
    if (modelVenda.fieldIndex("idVendaBase") == column) { continue; }
    if (modelVenda.fieldIndex("created") == column) { continue; }
    if (modelVenda.fieldIndex("lastUpdated") == column) { continue; }

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
  const double quant = modelProdutos2.data(mapperProdutos.currentIndex(), "quant").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double restante = quant - quantDevolvida;
  const double step = ui->doubleSpinBoxQuant->singleStep();

  // copiar linha para devolucao
  const int rowDevolucao = modelDevolvidos1.insertRowAtEnd();

  for (int column = 0; column < modelProdutos2.columnCount(); ++column) {
    if (modelProdutos2.fieldIndex("idVendaProduto2") == column) { continue; }
    if (modelProdutos2.fieldIndex("idVendaProdutoFK") == column) { continue; }
    if (modelProdutos2.fieldIndex("idNFeSaida") == column) { continue; }
    if (modelProdutos2.fieldIndex("created") == column) { continue; }
    if (modelProdutos2.fieldIndex("lastUpdated") == column) { continue; }

    const QString fieldNameSrc = modelProdutos2.record().fieldName(column);
    const int fieldNameDest = modelDevolvidos1.fieldIndex(fieldNameSrc);

    if (fieldNameDest == -1) { continue; }

    const QVariant value = modelProdutos2.data(currentRow, column);

    if (not modelDevolvidos1.setData(rowDevolucao, fieldNameDest, value)) { return false; }
  }

  if (not modelDevolvidos1.setData(rowDevolucao, "idVenda", idDevolucao)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "idRelacionado", modelProdutos2.data(currentRow, "idVendaProdutoFK"))) { return false; }

  const double quantDevolvidaInvertida = quantDevolvida * -1;
  const double parcialDevolvido = ui->doubleSpinBoxTotalItem->value() * -1;
  const double prcUnitarioDevolvido = parcialDevolvido / quantDevolvidaInvertida;

  if (not modelDevolvidos1.setData(rowDevolucao, "status", "PENDENTE DEV.")) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "statusOriginal", modelProdutos2.data(currentRow, "status"))) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "prcUnitario", prcUnitarioDevolvido)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "descUnitario", prcUnitarioDevolvido)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "caixas", (quantDevolvida / step * -1))) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "quant", quantDevolvidaInvertida)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "parcial", parcialDevolvido)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "desconto", 0)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "parcialDesc", ui->doubleSpinBoxTotalItem->value() * -1)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "descGlobal", 0)) { return false; }
  if (not modelDevolvidos1.setData(rowDevolucao, "total", ui->doubleSpinBoxTotalItem->value() * -1)) { return false; }

  if (not modelDevolvidos1.submitAll()) { return false; } // this copy into venda_has_produto2

  // get modelDevolvidos lastInsertId for modelConsumos
  const QString idVendaProduto1_Devolucao = modelDevolvidos1.query().lastInsertId().toString();

  QSqlQuery queryBusca;

  if (not queryBusca.exec("SELECT idVendaProduto2 FROM venda_has_produto2 WHERE idVendaProdutoFK = " + idVendaProduto1_Devolucao) or not queryBusca.first()) {
    return qApp->enqueueError(false, "Erro buscando idVendaProduto2: " + queryBusca.lastError().text(), this);
  }

  //------------------------------------

  modelConsumos.setFilter("idVendaProduto2 = " + modelProdutos2.data(currentRow, "idVendaProduto2").toString());

  if (not modelConsumos.select()) { return qApp->enqueueError(false, "Erro lendo consumos: " + modelConsumos.lastError().text(), this); }

  if (modelConsumos.rowCount() > 0) {
    if (not modelConsumos.setData(0, "idVendaProduto2", queryBusca.value("idVendaProduto2"))) { return false; }
    if (not modelConsumos.setData(0, "status", "PENDENTE DEV.")) { return false; }
    if (not modelConsumos.setData(0, "quant", quantDevolvida * -1)) { return false; }
    if (not modelConsumos.setData(0, "quantUpd", 5)) { return false; }
    if (not modelConsumos.setData(0, "caixas", (quantDevolvida / step))) { return false; }
  }

  //------------------------------------

  if (restante > 0) {
    const int newRowRestante = modelProdutos2.insertRowAtEnd();
    // NOTE: *quebralinha venda_produto2

    for (int column = 0; column < modelProdutos2.columnCount(); ++column) {
      if (modelProdutos2.fieldIndex("idVendaProduto2") == column) { continue; }
      if (modelProdutos2.fieldIndex("created") == column) { continue; }
      if (modelProdutos2.fieldIndex("lastUpdated") == column) { continue; }

      const QVariant value = modelProdutos2.data(currentRow, column);

      if (not modelProdutos2.setData(newRowRestante, column, value)) { return false; }
    }

    if (not modelProdutos2.setData(newRowRestante, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
    if (not modelProdutos2.setData(newRowRestante, "idRelacionado", modelProdutos2.data(currentRow, "idVendaProduto2"))) { return false; }
    if (not modelProdutos2.setData(newRowRestante, "caixas", (restante / step))) { return false; }
    if (not modelProdutos2.setData(newRowRestante, "quant", restante)) { return false; }

    const double prcUnitario = modelProdutos2.data(newRowRestante, "prcUnitario").toDouble();
    const double parcial = restante * prcUnitario;

    if (not modelProdutos2.setData(newRowRestante, "parcial", parcial)) { return false; }

    const double desconto = modelProdutos2.data(newRowRestante, "desconto").toDouble();
    const double parcialDesc = parcial * (1 - (desconto / 100));

    if (not modelProdutos2.setData(newRowRestante, "parcialDesc", parcialDesc)) { return false; }

    const double descGlobal = modelProdutos2.data(newRowRestante, "descGlobal").toDouble();
    const double total = parcialDesc * (1 - (descGlobal / 100));

    if (not modelProdutos2.setData(newRowRestante, "total", total)) { return false; }

    //------------------------------------

    if (modelConsumos.rowCount() > 0) {
      const int newRowConsumo = modelConsumos.insertRowAtEnd();

      for (int column = 0; column < modelConsumos.columnCount(); ++column) {
        if (modelConsumos.fieldIndex("idConsumo") == column) { continue; }
        if (modelConsumos.fieldIndex("created") == column) { continue; }
        if (modelConsumos.fieldIndex("lastUpdated") == column) { continue; }

        const QVariant value = modelConsumos.data(0, column);

        if (not modelConsumos.setData(newRowConsumo, column, value)) { return false; }
      }

      if (not modelConsumos.setData(newRowConsumo, "idVendaProduto2", novoIdVendaProduto2)) { return false; }
      if (not modelConsumos.setData(newRowConsumo, "status", "CONSUMO")) { return false; }
      if (not modelConsumos.setData(newRowConsumo, "quant", restante * -1)) { return false; }
      if (not modelConsumos.setData(newRowConsumo, "quantUpd", 4)) { return false; }
      if (not modelConsumos.setData(newRowConsumo, "caixas", (restante / step))) { return false; }
    }
  }

  // ------------------------------------

  // TODO: manter o restante na linha original para manter o vinculo vp<->consumo?
  // *Deve deixar a devolucao numa linha nova para seguir a ordem das operações
  // *Operações devem ficar na ordem que aconteceram

  const double prcUnitario = modelProdutos2.data(currentRow, "prcUnitario").toDouble();
  const double desconto = modelProdutos2.data(currentRow, "desconto").toDouble();
  const double descGlobal = modelProdutos2.data(currentRow, "descGlobal").toDouble();

  // linha original

  const double caixas = quantDevolvida / step;

  if (not modelProdutos2.setData(currentRow, "caixas", caixas)) { return false; }
  if (not modelProdutos2.setData(currentRow, "quant", quantDevolvida)) { return false; }

  const double parcialRestante = quantDevolvida * prcUnitario;

  if (not modelProdutos2.setData(currentRow, "parcial", parcialRestante)) { return false; }

  const double parcialDescRestante = parcialRestante * (1 - (desconto / 100));

  if (not modelProdutos2.setData(currentRow, "parcialDesc", parcialDescRestante)) { return false; }

  const double totalRestante = parcialDescRestante * (1 - (descGlobal / 100));

  // TODO: guardar status original nessa linha?
  if (not modelProdutos2.setData(currentRow, "total", totalRestante)) { return false; }
  if (not modelProdutos2.setData(currentRow, "status", "DEVOLVIDO")) { return false; }

  //----------------------------------------------

  if (not modelProdutos2.submitAll()) { return false; }

  if (not modelConsumos.submitAll()) { return false; }

  return true;
}

bool Devolucao::criarContas() {
  // TODO: 0considerar a 'conta cliente' e ajustar as telas do financeiro para poder visualizar/trabalhar os dois fluxos
  // de contas

  if (qFuzzyIsNull(ui->doubleSpinBoxCredito->value())) { return true; }

  const int newRowPag = modelPagamentos.insertRowAtEnd();

  if (not modelPagamentos.setData(newRowPag, "contraParte", modelCliente.data(0, "nome_razao"))) { return false; }
  if (not modelPagamentos.setData(newRowPag, "dataEmissao", qApp->serverDate())) { return false; }
  if (not modelPagamentos.setData(newRowPag, "idVenda", idDevolucao)) { return false; }
  if (not modelPagamentos.setData(newRowPag, "idLoja", UserSession::idLoja())) { return false; }
  if (not modelPagamentos.setData(newRowPag, "valor", ui->doubleSpinBoxCredito->value() * -1)) { return false; }
  if (not modelPagamentos.setData(newRowPag, "tipo", "1. Conta Cliente")) { return false; }
  if (not modelPagamentos.setData(newRowPag, "parcela", 1)) { return false; }
  if (not modelPagamentos.setData(newRowPag, "observacao", "")) { return false; }
  if (not modelPagamentos.setData(newRowPag, "status", "RECEBIDO")) { return false; }
  if (not modelPagamentos.setData(newRowPag, "dataPagamento", qApp->serverDate())) { return false; }
  if (not modelPagamentos.setData(newRowPag, "dataRealizado", qApp->serverDate())) { return false; }
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
  //    modelPagamentos.setData(row, "dataEmissao", qApp->serverDate());
  //    modelPagamentos.setData(row, "idVenda", idDevolucao);
  //    modelPagamentos.setData(row, "idLoja", modelVenda.data(0, "idLoja"));
  // TODO: 0colocar mesma data do fluxo original
  //    modelPagamentos.setData(row, "dataPagamento", qApp->serverDate());
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
  if (qFuzzyIsNull(ui->doubleSpinBoxCredito->value())) { return true; }

  const double credito = modelCliente.data(0, "credito").toDouble() + ui->doubleSpinBoxCredito->value();

  if (not modelCliente.setData(0, "credito", credito)) { return false; }

  if (not modelCliente.submitAll()) { return false; }

  return true;
}

bool Devolucao::devolverItem(const int currentRow, const bool createNewId, const int novoIdVendaProduto2) {
  if (createNewId and not criarDevolucao()) { return false; }

  if (not criarContas()) { return false; }
  if (not salvarCredito()) { return false; }
  if (not inserirItens(currentRow, novoIdVendaProduto2)) { return false; }
  if (not atualizarDevolucao()) { return false; }

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

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuant->value())) { return qApp->enqueueError("Não selecionou quantidade!", this); }

  const auto createNewId = determinarIdDevolucao();

  if (not createNewId) { return qApp->enqueueError("Não foi possível determinar o código da devolução!", this); }

  const auto novoIdVendaProduto2 = reservarIdVendaProduto2();

  if (not novoIdVendaProduto2) { return; }

  const QString idVenda = modelProdutos2.data(list.first().row(), "idVenda").toString();

  if (not qApp->startTransaction()) { return; }

  if (not devolverItem(list.first().row(), createNewId.value(), novoIdVendaProduto2.value())) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

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

void Devolucao::on_doubleSpinBoxTotalItem_valueChanged(double value) {
  ui->doubleSpinBoxCredito->setMaximum(value);
  ui->doubleSpinBoxCredito->setValue(value);
}

std::optional<int> Devolucao::reservarIdVendaProduto2() {
  if (qApp->getInTransaction()) {
    qApp->enqueueError("Erro ALTER TABLE durante transação!", this);
    return {};
  }

  QSqlQuery query;

  if (not query.exec("SELECT auto_increment FROM information_schema.tables WHERE table_schema = 'staccato' AND table_name = 'venda_has_produto2'") or not query.first()) {
    qApp->enqueueError("Erro reservar id venda: " + query.lastError().text(), this);
    return {};
  }

  const int id = query.value("auto_increment").toInt();

  if (not query.exec("ALTER TABLE venda_has_produto2 auto_increment = " + QString::number(id + 1))) {
    qApp->enqueueError("Erro reservar id venda: " + query.lastError().text(), this);
    return {};
  }

  return id;
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
// TODO: verificar se o valor total de um produto e o valor creditado devem ser sempre iguais

// TODO: lidar com os casos em que o produto estava agendado e é feita a devolucao, alterando os consumos
