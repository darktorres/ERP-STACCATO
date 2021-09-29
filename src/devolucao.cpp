#include "devolucao.h"
#include "ui_devolucao.h"

#include "application.h"
#include "checkboxdelegate.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "user.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlRecord>
#include <cmath>

Devolucao::Devolucao(const QString &idVenda, const bool isRepresentacao, QWidget *parent) : QDialog(parent), isRepresentacao(isRepresentacao), idVenda(idVenda), ui(new Ui::Devolucao) {
  ui->setupUi(this);

  setConnections();

  setWindowFlags(Qt::Window);

  setWindowTitle(idVenda);

  if (isRepresentacao) {
    ui->doubleSpinBoxPorcentagem->setDisabled(true);
    ui->doubleSpinBoxCredito->setDisabled(true);
  }

  setupTables();
}

Devolucao::~Devolucao() { delete ui; }

void Devolucao::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->doubleSpinBoxCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCaixas_valueChanged, connectionType);
  connect(ui->doubleSpinBoxQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxQuant_valueChanged, connectionType);
  connect(ui->doubleSpinBoxCredito, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCredito_valueChanged, connectionType);
  connect(ui->doubleSpinBoxPorcentagem, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxPorcentagem_valueChanged, connectionType);
  connect(ui->pushButtonDevolverItem, &QPushButton::clicked, this, &Devolucao::on_pushButtonDevolverItem_clicked, connectionType);
  connect(ui->tableProdutos, &TableView::clicked, this, &Devolucao::on_tableProdutos_clicked, connectionType);
}

void Devolucao::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->doubleSpinBoxCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCaixas_valueChanged);
  disconnect(ui->doubleSpinBoxQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxQuant_valueChanged);
  disconnect(ui->doubleSpinBoxCredito, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxCredito_valueChanged);
  disconnect(ui->doubleSpinBoxPorcentagem, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Devolucao::on_doubleSpinBoxPorcentagem_valueChanged);
  disconnect(ui->pushButtonDevolverItem, &QPushButton::clicked, this, &Devolucao::on_pushButtonDevolverItem_clicked);
  disconnect(ui->tableProdutos, &TableView::clicked, this, &Devolucao::on_tableProdutos_clicked);
}

void Devolucao::determinarIdDevolucao() {
  SqlQuery query;
  query.prepare("SELECT idVenda FROM venda WHERE idVenda LIKE :idVenda AND MONTH(data) = MONTH(CURDATE()) AND YEAR(data) = YEAR(CURDATE())");
  query.bindValue(":idVenda", idVenda + "D%");

  if (not query.exec()) { throw RuntimeException("Erro verificando se existe devolução: " + query.lastError().text()); }

  if (query.first()) {
    idDevolucao = query.value("idVenda").toString();
  } else {
    criarDevolucao();
  }
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

  modelProdutos2.select();

  ui->tableProdutos->setModel(&modelProdutos2);

  ui->tableProdutos->hideColumn("idNFeEntrada");
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

  modelDevolvidos1.select();

  ui->tableDevolvidos->setModel(&modelDevolvidos1);

  ui->tableDevolvidos->hideColumn("idNFeEntrada");
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

  modelPagamentos.select();

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
  ui->tablePagamentos->hideColumn("idConta");
  ui->tablePagamentos->hideColumn("tipoDet");
  ui->tablePagamentos->hideColumn("centroCusto");
  ui->tablePagamentos->hideColumn("grupo");
  ui->tablePagamentos->hideColumn("subGrupo");
  ui->tablePagamentos->hideColumn("taxa");
  ui->tablePagamentos->hideColumn("contraParte");
  ui->tablePagamentos->hideColumn("comissao");
  ui->tablePagamentos->hideColumn("desativado");

  ui->tablePagamentos->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePagamentos->setItemDelegateForColumn("representacao", new CheckBoxDelegate(true, this));

  ui->tablePagamentos->setPersistentColumns({"representacao"});

  //--------------------------------------------------------------

  modelVenda.setTable("venda");

  modelVenda.setFilter("idVenda = '" + idVenda + "'");

  modelVenda.select();

  //--------------------------------------------------------------

  modelCliente.setTable("cliente");

  modelCliente.setFilter("idCliente = " + modelVenda.data(0, "idCliente").toString());

  modelCliente.select();

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

  if (isRepresentacao) {
    ui->doubleSpinBoxPorcentagem->setValue(0);
    ui->doubleSpinBoxCredito->setValue(0);
  }

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

void Devolucao::criarDevolucao() {
  SqlQuery query2;
  query2.prepare("SELECT COALESCE(RIGHT(MAX(IDVENDA), 1) + 1, 1) AS number FROM venda WHERE idVenda LIKE :idVenda");
  query2.bindValue(":idVenda", idVenda + "D%");

  if (not query2.exec()) { throw RuntimeException("Erro determinando próximo id: " + query2.lastError().text()); }

  if (not query2.first()) { throw RuntimeException("Não encontrou próximo idVenda para id: " + idVenda); }

  idDevolucao = idVenda + "D" + query2.value("number").toString();

  // -------------------------------------------------------------------------

  const int newRow = modelVenda.insertRowAtEnd();

  for (int column = 0, columnCount = modelVenda.columnCount(); column < columnCount; ++column) {
    if (column == modelVenda.fieldIndex("idVendaBase")) { continue; }
    if (column == modelVenda.fieldIndex("created")) { continue; }
    if (column == modelVenda.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelVenda.data(0, column);

    if (value.isNull()) { continue; }

    modelVenda.setData(newRow, column, value);
  }

  modelVenda.setData(newRow, "idVenda", idDevolucao);
  modelVenda.setData(newRow, "data", qApp->serverDateTime());
  modelVenda.setData(newRow, "subTotalBru", 0);
  modelVenda.setData(newRow, "subTotalLiq", 0);
  modelVenda.setData(newRow, "descontoPorc", 0);
  modelVenda.setData(newRow, "descontoReais", 0);
  modelVenda.setData(newRow, "frete", 0);
  modelVenda.setData(newRow, "total", 0);
  modelVenda.setData(newRow, "prazoEntrega", 0);
  modelVenda.setData(newRow, "devolucao", true);
  modelVenda.setData(newRow, "status", "DEVOLVIDO");

  modelVenda.submitAll();
}

void Devolucao::desvincularCompra(const QString &idVendaProduto2) {
  SqlQuery query;

  if (not query.exec("UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, idVendaProduto2 = NULL WHERE idVendaProduto2 = " + idVendaProduto2)) {
    throw RuntimeException("Erro desvinculando da compra: " + query.lastError().text());
  }
}

void Devolucao::inserirItens(const int currentRow, const int novoIdVendaProduto2) {
  copiarProdutoParaDevolucao(currentRow);

  //------------------------------------

  const double quantTotal = modelProdutos2.data(currentRow, "quant").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double restante = quantTotal - quantDevolvida;

  if (restante > 0) {
    dividirVenda(currentRow, novoIdVendaProduto2);
    dividirCompra(currentRow, novoIdVendaProduto2);
    dividirConsumo(currentRow, novoIdVendaProduto2);
  }

  //------------------------------------

  alterarLinhaOriginal(currentRow);
  desvincularCompra(modelProdutos2.data(currentRow, "idVendaProduto2").toString());

  //------------------------------------

  modelProdutos2.submitAll();

  modelCompra.submitAll();

  modelConsumos.submitAll();
}

void Devolucao::criarComissaoProfissional(const int currentRow) {
  const QDate date = qApp->serverDate();
  const QString idVendaProduto2 = modelProdutos2.data(currentRow, "idVendaProduto2").toString();

  SqlQuery queryProfissional;

  if (not queryProfissional.exec("SELECT p.nome_razao, v.rt FROM profissional p LEFT JOIN venda v ON p.idProfissional = v.idProfissional LEFT JOIN venda_has_produto2 vp2 ON v.idVenda = vp2.idVenda "
                                 "WHERE vp2.idVendaProduto2 = " +
                                 idVendaProduto2)) {
    throw RuntimeException("Erro buscando dados do profissional: " + queryProfissional.lastError().text());
  }

  if (not queryProfissional.first()) { throw RuntimeException("Dados não encontrados para profissional com id: " + idVendaProduto2); }

  const double rt = queryProfissional.value("rt").toDouble() / 100;
  const double valor = modelProdutos2.data(currentRow, "total").toDouble() * -1 * rt;
  const QString profissional = queryProfissional.value("nome_razao").toString();

  if (not qFuzzyIsNull(valor)) {
    SqlQuery query1;
    query1.prepare("INSERT INTO conta_a_pagar_has_pagamento (dataEmissao, idVenda, contraParte, idLoja, idConta, centroCusto, valor, tipo, dataPagamento, grupo) "
                   "VALUES (:dataEmissao, :idVenda, :contraParte, :idLoja, :idConta, :centroCusto, :valor, :tipo, :dataPagamento, :grupo)");
    query1.bindValue(":dataEmissao", date);
    query1.bindValue(":idVenda", idDevolucao);
    query1.bindValue(":contraParte", profissional);
    query1.bindValue(":idLoja", modelProdutos2.data(currentRow, "idLoja"));
    query1.bindValue(":idConta", 8); // caixa matriz
    query1.bindValue(":centroCusto", modelProdutos2.data(currentRow, "idLoja"));
    query1.bindValue(":valor", valor);
    query1.bindValue(":tipo", "1. Dinheiro");
    // 01-15 paga dia 30, 16-30 paga prox dia 15
    QDate quinzena1 = QDate(date.year(), date.month(), qMin(date.daysInMonth(), 30));
    QDate quinzena2 = date.addMonths(1);
    quinzena2.setDate(quinzena2.year(), quinzena2.month(), 15);
    query1.bindValue(":dataPagamento", date.day() <= 15 ? quinzena1 : quinzena2);
    query1.bindValue(":grupo", "RT's");

    if (not query1.exec()) { throw RuntimeException("Erro cadastrando RT: " + query1.lastError().text()); }
  }
}

void Devolucao::criarContas() {
  // TODO: 0considerar a 'conta cliente' e ajustar as telas do financeiro para poder visualizar/trabalhar os dois fluxos de contas

  if (qFuzzyIsNull(ui->doubleSpinBoxCredito->value())) { return; }

  //--------------------------------------------------------------

  const int newRow = modelPagamentos.insertRowAtEnd();

  modelPagamentos.setData(newRow, "contraParte", modelCliente.data(0, "nome_razao"));
  modelPagamentos.setData(newRow, "dataEmissao", qApp->serverDate());
  modelPagamentos.setData(newRow, "idVenda", idDevolucao);
  // TODO: porque usa o idLoja do usuario?
  modelPagamentos.setData(newRow, "idLoja", User::idLoja);
  modelPagamentos.setData(newRow, "valor", ui->doubleSpinBoxCredito->value() * -1);
  modelPagamentos.setData(newRow, "tipo", "1. Conta Cliente");
  modelPagamentos.setData(newRow, "parcela", 1);
  modelPagamentos.setData(newRow, "observacao", "");
  modelPagamentos.setData(newRow, "status", "RECEBIDO");
  modelPagamentos.setData(newRow, "dataPagamento", qApp->serverDate());
  modelPagamentos.setData(newRow, "dataRealizado", qApp->serverDate());
  modelPagamentos.setData(newRow, "valorReal", ui->doubleSpinBoxCredito->value() * -1);

  const int contaCreditos = 11;
  modelPagamentos.setData(newRow, "idConta", contaCreditos);

  modelPagamentos.submitAll();
}

void Devolucao::salvarCredito() {
  if (qFuzzyIsNull(ui->doubleSpinBoxCredito->value())) { return; }

  const double credito = modelCliente.data(0, "credito").toDouble() + ui->doubleSpinBoxCredito->value();

  modelCliente.setData(0, "credito", credito);

  modelCliente.submitAll();
}

void Devolucao::devolverItem(const int currentRow, const int novoIdVendaProduto2) {
  determinarIdDevolucao();

  criarComissaoProfissional(currentRow);
  criarContas();
  salvarCredito();
  inserirItens(currentRow, novoIdVendaProduto2);
  atualizarDevolucao();

  Sql::updateVendaStatus(idVenda);
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

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuant->value())) { throw RuntimeError("Não selecionou quantidade!", this); }

  // -------------------------------------------------------------------------

  const int novoIdVendaProduto2 = qApp->reservarIdVendaProduto2();

  // -------------------------------------------------------------------------

  qApp->startTransaction("Devolucao::on_pushButtonDevolverItem");

  devolverItem(list.first().row(), novoIdVendaProduto2);

  qApp->endTransaction();

  // -------------------------------------------------------------------------

  limparCampos();

  qApp->enqueueInformation("Devolução realizada com sucesso!", this);
}

void Devolucao::atualizarDevolucao() {
  SqlQuery query;
  query.prepare("SELECT SUM(parcial) AS parcial, SUM(parcialDesc) AS parcialDesc FROM venda_has_produto2 WHERE idVenda = :idVenda");
  query.bindValue(":idVenda", idDevolucao);

  if (not query.exec()) { throw RuntimeException("Erro buscando dados da devolução: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados para devolução com id: " + idDevolucao); }

  SqlQuery query2;
  query2.prepare("UPDATE venda SET subTotalBru = :subTotalBru, subTotalLiq = :subTotalLiq, total = :total WHERE idVenda = :idVenda");
  query2.bindValue(":subTotalBru", query.value("parcial"));
  query2.bindValue(":subTotalLiq", query.value("parcialDesc"));
  query2.bindValue(":total", query.value("parcialDesc"));
  query2.bindValue(":idVenda", idDevolucao);

  if (not query2.exec()) { throw RuntimeException("Erro atualizando devolução: " + query2.lastError().text()); }
}

void Devolucao::copiarProdutoParaDevolucao(const int currentRow) {
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

    if (value.isNull()) { continue; }

    modelDevolvidos1.setData(newRow, columnIndex, value);
  }

  //--------------------------------------------------------------

  const double kgcx = modelProdutos2.data(currentRow, "kg").toDouble() / modelProdutos2.data(currentRow, "caixas").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double caixas = quantDevolvida / stepQuant * -1;
  const double quantDevolvidaInvertida = quantDevolvida * -1;
  // TODO: isso não é o mesmo que pegar descUnitario * -1?
  const double prcUnitario = modelProdutos2.data(currentRow, "total").toDouble() / modelProdutos2.data(currentRow, "quant").toDouble() * -1;
  const double total = prcUnitario * quantDevolvida;

  modelDevolvidos1.setData(newRow, "idVenda", idDevolucao);
  modelDevolvidos1.setData(newRow, "idRelacionado", modelProdutos2.data(currentRow, "idVendaProdutoFK"));
  modelDevolvidos1.setData(newRow, "status", "PENDENTE DEV.");
  modelDevolvidos1.setData(newRow, "statusOriginal", modelProdutos2.data(currentRow, "status"));
  modelDevolvidos1.setData(newRow, "prcUnitario", prcUnitario);
  modelDevolvidos1.setData(newRow, "descUnitario", prcUnitario);
  modelDevolvidos1.setData(newRow, "caixas", caixas);
  modelDevolvidos1.setData(newRow, "kg", caixas * kgcx * -1);
  modelDevolvidos1.setData(newRow, "quant", quantDevolvidaInvertida);
  modelDevolvidos1.setData(newRow, "parcial", total);
  modelDevolvidos1.setData(newRow, "desconto", 0);
  modelDevolvidos1.setData(newRow, "parcialDesc", total);
  modelDevolvidos1.setData(newRow, "descGlobal", 0);
  modelDevolvidos1.setData(newRow, "total", total);

  modelDevolvidos1.submitAll(); // SQL trigger copies into venda_has_produto2

  //--------------------------------------------------------------

  atualizarIdRelacionado(currentRow);
}

void Devolucao::atualizarIdRelacionado(const int currentRow) {
  // get modelDevolvidos lastInsertId for modelConsumos
  const QString idVendaProduto1_Devolucao = modelDevolvidos1.query().lastInsertId().toString();

  SqlQuery queryBusca;

  if (not queryBusca.exec("SELECT idVendaProduto2 FROM venda_has_produto2 WHERE idVendaProdutoFK = " + idVendaProduto1_Devolucao)) {
    throw RuntimeException("Erro buscando idVendaProduto2: " + queryBusca.lastError().text());
  }

  if (not queryBusca.first()) { throw RuntimeException("Dados não encontrados para id: " + idVendaProduto1_Devolucao); }

  //------------------------------------

  SqlQuery queryRelacionado;

  if (not queryRelacionado.exec("UPDATE venda_has_produto2 SET idRelacionado = " + modelProdutos2.data(currentRow, "idVendaProduto2").toString() +
                                " WHERE idVendaProduto2 = " + queryBusca.value("idVendaProduto2").toString())) {
    throw RuntimeException("Erro marcando linha relacionada: " + queryRelacionado.lastError().text());
  }
}

void Devolucao::dividirVenda(const int currentRow, const int novoIdVendaProduto2) {
  // NOTE: *quebralinha venda_produto2
  const int newRow = modelProdutos2.insertRowAtEnd();

  for (int column = 0; column < modelProdutos2.columnCount(); ++column) {
    if (column == modelProdutos2.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelProdutos2.fieldIndex("created")) { continue; }
    if (column == modelProdutos2.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelProdutos2.data(currentRow, column);

    if (value.isNull()) { continue; }

    modelProdutos2.setData(newRow, column, value);
  }

  //--------------------------------------------------------------------

  const double kgcx = modelProdutos2.data(currentRow, "kg").toDouble() / modelProdutos2.data(currentRow, "caixas").toDouble();
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

  modelProdutos2.setData(newRow, "idVendaProduto2", novoIdVendaProduto2);
  modelProdutos2.setData(newRow, "idRelacionado", modelProdutos2.data(currentRow, "idVendaProduto2"));
  modelProdutos2.setData(newRow, "caixas", (restante / stepQuant));
  modelProdutos2.setData(newRow, "kg", (restante / stepQuant) * kgcx);
  modelProdutos2.setData(newRow, "quant", restante);
  modelProdutos2.setData(newRow, "parcial", parcial);
  modelProdutos2.setData(newRow, "parcialDesc", parcialDesc);
  modelProdutos2.setData(newRow, "total", total);
}

void Devolucao::dividirCompra(const int currentRow, const int novoIdVendaProduto2) {
  modelCompra.setFilter("idVendaProduto2 = " + modelProdutos2.data(currentRow, "idVendaProduto2").toString());

  modelCompra.select();

  if (modelCompra.rowCount() == 0) { return; }

  //--------------------------------------------------------------------

  const double quantOriginal = modelCompra.data(0, "quant").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double prcUnitario = modelCompra.data(0, "prcUnitario").toDouble();
  const double quantRestante = quantOriginal - quantDevolvida;
  const QString status = modelCompra.data(0, "status").toString();
  // TODO: The 'idVenda' local variable possesses the same name as one of the class members, which can result in a confusion.
  const QString idVenda = modelCompra.data(0, "idVenda").toString();

  modelCompra.setData(0, "obs", idVenda + " DEVOLVEU");
  modelCompra.setData(0, "quant", quantDevolvida);
  modelCompra.setData(0, "caixas", quantDevolvida / stepQuant);
  modelCompra.setData(0, "preco", prcUnitario * quantDevolvida);

  //--------------------------------------------------------------------

  // NOTE: *quebralinha pedido_fornecedor2
  const int newRow = modelCompra.insertRowAtEnd();

  for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
    if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
    if (column == modelCompra.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelCompra.fieldIndex("obs")) { continue; }
    if (column == modelCompra.fieldIndex("created")) { continue; }
    if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelCompra.data(0, column);

    if (value.isNull()) { continue; }

    modelCompra.setData(newRow, column, value);
  }

  //--------------------------------------------------------------------

  modelCompra.setData(newRow, "status", status);
  modelCompra.setData(newRow, "idRelacionado", modelCompra.data(0, "idPedido2"));
  modelCompra.setData(newRow, "idVenda", idVenda);
  modelCompra.setData(newRow, "idVendaProduto2", novoIdVendaProduto2);
  modelCompra.setData(newRow, "quant", quantRestante);
  modelCompra.setData(newRow, "caixas", quantRestante / stepQuant);
  modelCompra.setData(newRow, "preco", prcUnitario * quantRestante);
}

void Devolucao::dividirConsumo(const int currentRow, const int novoIdVendaProduto2) {
  modelConsumos.setFilter("idVendaProduto2 = " + modelProdutos2.data(currentRow, "idVendaProduto2").toString());

  modelConsumos.select();

  if (modelConsumos.rowCount() == 0) { return; }

  //--------------------------------------------------------------------

  const double quantTotal = modelProdutos2.data(currentRow, "quant").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double caixasDevolvida = ui->doubleSpinBoxCaixas->value();

  const double valorConsumo = modelConsumos.data(0, "valor").toDouble();
  const double quantTrib = modelConsumos.data(0, "quantTrib").toDouble();
  const double desconto = modelConsumos.data(0, "desconto").toDouble();
  const double vBC = modelConsumos.data(0, "vBC").toDouble();
  const double vICMS = modelConsumos.data(0, "vICMS").toDouble();
  const double vBCST = modelConsumos.data(0, "vBCST").toDouble();
  const double vICMSST = modelConsumos.data(0, "vICMSST").toDouble();
  const double vBCPIS = modelConsumos.data(0, "vBCPIS").toDouble();
  const double vPIS = modelConsumos.data(0, "vPIS").toDouble();
  const double vBCCOFINS = modelConsumos.data(0, "vBCCOFINS").toDouble();
  const double vCOFINS = modelConsumos.data(0, "vCOFINS").toDouble();

  const double proporcao = quantDevolvida / quantTotal;

  modelConsumos.setData(0, "quant", quantDevolvida * -1);
  modelConsumos.setData(0, "caixas", caixasDevolvida);
  modelConsumos.setData(0, "valor", valorConsumo * proporcao);

  // impostos
  modelConsumos.setData(0, "quantTrib", quantTrib * proporcao);
  modelConsumos.setData(0, "desconto", desconto * proporcao);
  modelConsumos.setData(0, "vBC", vBC * proporcao);
  modelConsumos.setData(0, "vICMS", vICMS * proporcao);
  modelConsumos.setData(0, "vBCST", vBCST * proporcao);
  modelConsumos.setData(0, "vICMSST", vICMSST * proporcao);
  modelConsumos.setData(0, "vBCPIS", vBCPIS * proporcao);
  modelConsumos.setData(0, "vPIS", vPIS * proporcao);
  modelConsumos.setData(0, "vBCCOFINS", vBCCOFINS * proporcao);
  modelConsumos.setData(0, "vCOFINS", vCOFINS * proporcao);

  //--------------------------------------------------------------------

  // NOTE: *quebralinha estoque_consumo
  const int newRow = modelConsumos.insertRowAtEnd();

  for (int column = 0; column < modelConsumos.columnCount(); ++column) {
    if (column == modelConsumos.fieldIndex("idConsumo")) { continue; }
    if (column == modelConsumos.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelConsumos.fieldIndex("created")) { continue; }
    if (column == modelConsumos.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelConsumos.data(0, column);

    if (value.isNull()) { continue; }

    modelConsumos.setData(newRow, column, value);
  }

  //--------------------------------------------------------------------

  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double restante = quantTotal - quantDevolvida;

  const double proporcaoNovo = restante / quantTotal;

  modelConsumos.setData(newRow, "idVendaProduto2", novoIdVendaProduto2);
  modelConsumos.setData(newRow, "quant", restante * -1);
  modelConsumos.setData(newRow, "caixas", (restante / stepQuant));
  modelConsumos.setData(newRow, "valor", valorConsumo * proporcaoNovo);

  // impostos
  modelConsumos.setData(newRow, "quantTrib", quantTrib * proporcaoNovo);
  modelConsumos.setData(newRow, "desconto", desconto * proporcaoNovo);
  modelConsumos.setData(newRow, "vBC", vBC * proporcaoNovo);
  modelConsumos.setData(newRow, "vICMS", vICMS * proporcaoNovo);
  modelConsumos.setData(newRow, "vBCST", vBCST * proporcaoNovo);
  modelConsumos.setData(newRow, "vICMSST", vICMSST * proporcaoNovo);
  modelConsumos.setData(newRow, "vBCPIS", vBCPIS * proporcaoNovo);
  modelConsumos.setData(newRow, "vPIS", vPIS * proporcaoNovo);
  modelConsumos.setData(newRow, "vBCCOFINS", vBCCOFINS * proporcaoNovo);
  modelConsumos.setData(newRow, "vCOFINS", vCOFINS * proporcaoNovo);
}

void Devolucao::alterarLinhaOriginal(const int currentRow) {
  const double kgcx = modelProdutos2.data(currentRow, "kg").toDouble() / modelProdutos2.data(currentRow, "caixas").toDouble();
  const double prcUnitario = modelProdutos2.data(currentRow, "prcUnitario").toDouble();
  const double desconto = modelProdutos2.data(currentRow, "desconto").toDouble();
  const double descGlobal = modelProdutos2.data(currentRow, "descGlobal").toDouble();
  const double quantDevolvida = ui->doubleSpinBoxQuant->value();
  const double stepQuant = ui->doubleSpinBoxQuant->singleStep();
  const double caixas = quantDevolvida / stepQuant;
  const double parcialRestante = quantDevolvida * prcUnitario;
  const double parcialDescRestante = parcialRestante * (1 - (desconto / 100));
  const double totalRestante = parcialDescRestante * (1 - (descGlobal / 100));

  modelProdutos2.setData(currentRow, "caixas", caixas);
  modelProdutos2.setData(currentRow, "kg", caixas * kgcx);
  modelProdutos2.setData(currentRow, "quant", quantDevolvida);
  modelProdutos2.setData(currentRow, "parcial", parcialRestante);
  modelProdutos2.setData(currentRow, "parcialDesc", parcialDescRestante);
  modelProdutos2.setData(currentRow, "total", totalRestante);
  modelProdutos2.setData(currentRow, "statusOriginal", modelProdutos2.data(currentRow, "status"));
  modelProdutos2.setData(currentRow, "status", "DEVOLVIDO");
}

// TODO: 0. lidar com os casos em que o produto estava agendado e é feita a devolucao, alterando os consumos
// TODO: 1. perguntar e guardar data em que ocorreu a devolucao
// TODO: 2. ??? nao criar linha conta
// TODO: 2. adicionar devolucao de frete quando houver
// TODO: 2. criar linha no followup
// TODO: 2. quando for devolver para o fornecedor perguntar a quantidade
// TODO: ativar ordenação nas tabelas?
