#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <utility>

#include "doubledelegate.h"
#include "estoque.h"
#include "estoqueproxymodel.h"
#include "ui_estoque.h"
#include "xml_viewer.h"

Estoque::Estoque(QString idEstoque, const bool showWindow, QWidget *parent) : QDialog(parent), idEstoque(std::move(idEstoque)), ui(new Ui::Estoque) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  connect(ui->pushButtonExibirNfe, &QPushButton::clicked, this, &Estoque::on_pushButtonExibirNfe_clicked);
  connect(ui->tableConsumo, &TableView::entered, this, &Estoque::on_tableConsumo_entered);
  connect(ui->tableEstoque, &TableView::activated, this, &Estoque::on_tableEstoque_activated);
  connect(ui->tableEstoque, &TableView::entered, this, &Estoque::on_tableEstoque_entered);

  viewRegisterById(showWindow);
}

Estoque::~Estoque() { delete ui; }

void Estoque::setupTables() {
  model.setTable("estoque");
  model.setEditStrategy(QSqlTableModel::OnManualSubmit);

  model.setHeaderData("idEstoque", "Estoque");
  model.setHeaderData("recebidoPor", "Recebido Por");
  model.setHeaderData("status", "Status");
  model.setHeaderData("local", "Local");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  model.setHeaderData("quant", "Quant.");
  model.setHeaderData("un", "Un.");
  model.setHeaderData("caixas", "Caixas");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("lote", "Lote");
  model.setHeaderData("bloco", "Bloco");

  ui->tableEstoque->setModel(new EstoqueProxyModel(&model, this));
  ui->tableEstoque->setItemDelegateForColumn("quant", new DoubleDelegate(this, 4));
  ui->tableEstoque->hideColumn("quantUpd");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("codBarras");
  ui->tableEstoque->hideColumn("ncm");
  ui->tableEstoque->hideColumn("cfop");
  ui->tableEstoque->hideColumn("valorUnid");
  ui->tableEstoque->hideColumn("valor");
  ui->tableEstoque->hideColumn("codBarrasTrib");
  ui->tableEstoque->hideColumn("unTrib");
  ui->tableEstoque->hideColumn("quantTrib");
  ui->tableEstoque->hideColumn("valorTrib");
  ui->tableEstoque->hideColumn("desconto");
  ui->tableEstoque->hideColumn("compoeTotal");
  ui->tableEstoque->hideColumn("numeroPedido");
  ui->tableEstoque->hideColumn("itemPedido");
  ui->tableEstoque->hideColumn("tipoICMS");
  ui->tableEstoque->hideColumn("orig");
  ui->tableEstoque->hideColumn("cstICMS");
  ui->tableEstoque->hideColumn("modBC");
  ui->tableEstoque->hideColumn("vBC");
  ui->tableEstoque->hideColumn("pICMS");
  ui->tableEstoque->hideColumn("vICMS");
  ui->tableEstoque->hideColumn("modBCST");
  ui->tableEstoque->hideColumn("pMVAST");
  ui->tableEstoque->hideColumn("vBCST");
  ui->tableEstoque->hideColumn("pICMSST");
  ui->tableEstoque->hideColumn("vICMSST");
  ui->tableEstoque->hideColumn("cEnq");
  ui->tableEstoque->hideColumn("cstIPI");
  ui->tableEstoque->hideColumn("cstPIS");
  ui->tableEstoque->hideColumn("vBCPIS");
  ui->tableEstoque->hideColumn("pPIS");
  ui->tableEstoque->hideColumn("vPIS");
  ui->tableEstoque->hideColumn("cstCOFINS");
  ui->tableEstoque->hideColumn("vBCCOFINS");
  ui->tableEstoque->hideColumn("pCOFINS");
  ui->tableEstoque->hideColumn("vCOFINS");

  modelConsumo.setTable("estoque_has_consumo");
  modelConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelConsumo.setFilter("0");

  modelViewConsumo.setTable("view_estoque_consumo");
  modelViewConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelViewConsumo.setHeaderData("statusProduto", "Pedido");
  modelViewConsumo.setHeaderData("status", "Status");
  modelViewConsumo.setHeaderData("ordemCompra", "OC");
  modelViewConsumo.setHeaderData("local", "Local");
  modelViewConsumo.setHeaderData("fornecedor", "Fornecedor");
  modelViewConsumo.setHeaderData("descricao", "Produto");
  modelViewConsumo.setHeaderData("quant", "Quant.");
  modelViewConsumo.setHeaderData("un", "Un.");
  modelViewConsumo.setHeaderData("caixas", "Caixas");
  modelViewConsumo.setHeaderData("codComercial", "Cód. Com.");
  modelViewConsumo.setHeaderData("created", "Criado");

  ui->tableConsumo->setModel(new EstoqueProxyModel(&modelViewConsumo, this));
  ui->tableConsumo->setItemDelegateForColumn("quant", new DoubleDelegate(this, 4));
  ui->tableConsumo->showColumn("created");
  ui->tableConsumo->hideColumn("idEstoque");
  ui->tableConsumo->hideColumn("quantUpd");

  modelCompra.setTable("pedido_fornecedor_has_produto");
  modelCompra.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelCompra.setFilter("0");
}

void Estoque::on_tableEstoque_activated(const QModelIndex &) { exibirNota(); }

void Estoque::calcularRestante() {
  double quantRestante = model.data(0, "quant").toDouble();
  double quantComprometidoNaoEntregue = model.data(0, "quant").toDouble();

  for (int row = 0; row < modelViewConsumo.rowCount(); ++row) {
    const double quant = modelViewConsumo.data(row, "quant").toDouble();
    const QString statusProduto = modelViewConsumo.data(row, "statusProduto").toString();

    quantRestante += quant;

    if (statusProduto == "ENTREGUE") quantComprometidoNaoEntregue -= quant * -1;
  }

  const QString un = model.data(0, "un").toString();

  ui->doubleSpinBoxRestante->setValue(quantRestante);
  ui->doubleSpinBoxRestante->setSuffix(" " + un);

  ui->doubleSpinBoxComprometidoNaoEntregue->setValue(quantComprometidoNaoEntregue);
  ui->doubleSpinBoxComprometidoNaoEntregue->setSuffix(" " + un);
}

bool Estoque::viewRegisterById(const bool showWindow) {
  if (idEstoque.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Estoque não encontrado!");
    return false;
  }

  model.setFilter("idEstoque = " + idEstoque);

  if (not model.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela estoque: " + model.lastError().text());
    return false;
  }

  ui->tableEstoque->resizeColumnsToContents();

  if (not modelConsumo.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela estoque consumo: " + modelConsumo.lastError().text());
    return false;
  }

  modelViewConsumo.setFilter("idEstoque = " + idEstoque);

  if (not modelViewConsumo.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela view consumo: " + modelViewConsumo.lastError().text());
    return false;
  }

  ui->tableConsumo->resizeColumnsToContents();

  QSqlQuery query;
  query.prepare("SELECT pf.codComercial, pf.idCompra FROM estoque e, estoque_has_compra ehc, pedido_fornecedor_has_produto pf WHERE e.idEstoque = ehc.idEstoque AND ehc.idEstoque = :idEstoque AND "
                "pf.idCompra = ehc.idCompra AND pf.codComercial = e.codComercial");
  query.bindValue(":idEstoque", idEstoque);

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando compra relacionada ao estoque: " + query.lastError().text());
    return false;
  }

  const QString idCompra = query.value("idCompra").toString();
  const QString codComercial = query.value("codComercial").toString();

  modelCompra.setFilter("idCompra = " + idCompra + " AND codComercial = '" + codComercial + "'");

  if (not modelCompra.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela pedido_fornecedor: " + modelCompra.lastError().text());
    return false;
  }

  calcularRestante();

  if (showWindow) show();

  return true;
}

void Estoque::on_pushButtonExibirNfe_clicked() { exibirNota(); }

void Estoque::exibirNota() {
  QSqlQuery query;
  query.prepare("SELECT xml FROM estoque e LEFT JOIN estoque_has_nfe ehn ON e.idEstoque = ehn.idEstoque LEFT JOIN nfe n ON ehn.idNFe = n.idNFe WHERE e.idEstoque = :idEstoque");
  query.bindValue(":idEstoque", model.data(0, "idEstoque"));

  if (not query.exec()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando nfe: " + query.lastError().text());
    return;
  }

  while (query.next()) {
    auto *viewer = new XML_Viewer(this);
    viewer->exibirXML(query.value("xml").toByteArray());
  }
}

bool Estoque::atualizaQuantEstoque(const int idVendaProduto) {
  QSqlQuery query;
  query.prepare(
      "UPDATE venda_has_produto vhp, produto p SET p.estoqueRestante = p.estoqueRestante - vhp.quant WHERE vhp.idProduto = p.idProduto AND vhp.idVendaProduto = :idVendaProduto AND vhp.estoque > 0");
  query.bindValue(":idVendaProduto", idVendaProduto);

  if (not query.exec()) {
    QMessageBox::critical(this, "Erro!", "Erro atualizando quant. estoque: " + query.lastError().text());
    return false;
  }

  return true;
}

bool Estoque::criarConsumo(const int idVendaProduto, const double quant) {
  // TODO: [*** Conrado/Anderson] relacionar o consumo quebrando a linha em pedido_fornecedor_has_produto e setar o idVenda/idVendaProduto

  if (model.filter().isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Não setou idEstoque!");
    return false;
  }

  if (quant > ui->doubleSpinBoxRestante->value()) {
    QMessageBox::critical(this, "Erro!", "Quantidade insuficiente!");
    return false;
  }

  //

  if (not atualizaQuantEstoque(idVendaProduto)) return false;

  if (not quebrarCompra(idVendaProduto, quant)) return false;

  //

  const int row = 0;
  const int newRow = modelConsumo.rowCount();

  modelConsumo.insertRow(newRow);

  for (int column = 0, columnCount = model.columnCount(); column < columnCount; ++column) {
    const QString field = model.record().fieldName(column);
    const int index = modelConsumo.fieldIndex(field);
    const QVariant value = model.data(row, column);

    if (field == "created") continue;
    if (field == "lastUpdated") continue;

    if (index != -1 and not modelConsumo.setData(newRow, index, value)) return false;
  }

  QSqlQuery query;
  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", model.data(row, "idProduto"));

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando dados do produto: " + query.lastError().text());
    return false;
  }

  const QString un = query.value("un").toString();
  const double m2cx = query.value("m2cx").toDouble();
  const double pccx = query.value("pccx").toDouble();

  const double unCaixa = un == "M2" or un == "M²" or un == "ML" ? m2cx : pccx;

  const double caixas = qRound(quant / unCaixa * 100) / 100.;

  const double proporcao = quant / model.data(row, "quant").toDouble();

  const double valor = model.data(row, "valor").toDouble() * proporcao;
  const double vBC = model.data(row, "vBC").toDouble() * proporcao;
  const double vICMS = model.data(row, "vICMS").toDouble() * proporcao;
  const double vBCST = model.data(row, "vBCST").toDouble() * proporcao;
  const double vICMSST = model.data(row, "vICMSST").toDouble() * proporcao;
  const double vBCPIS = model.data(row, "vBCPIS").toDouble() * proporcao;
  const double vPIS = model.data(row, "vPIS").toDouble() * proporcao;
  const double vBCCOFINS = model.data(row, "vBCCOFINS").toDouble() * proporcao;
  const double vCOFINS = model.data(row, "vCOFINS").toDouble() * proporcao;

  // -------------------------------------

  if (not modelConsumo.setData(newRow, "quant", quant * -1)) return false;
  if (not modelConsumo.setData(newRow, "caixas", caixas)) return false;
  if (not modelConsumo.setData(newRow, "quantUpd", static_cast<int>(FieldColors::DarkGreen))) return false;
  if (not modelConsumo.setData(newRow, "idVendaProduto", idVendaProduto)) return false;
  if (not modelConsumo.setData(newRow, "idEstoque", model.data(row, "idEstoque"))) return false;
  if (not modelConsumo.setData(newRow, "status", "CONSUMO")) return false;

  if (not modelConsumo.setData(newRow, "valor", valor)) return false;
  if (not modelConsumo.setData(newRow, "vBC", vBC)) return false;
  if (not modelConsumo.setData(newRow, "vICMS", vICMS)) return false;
  if (not modelConsumo.setData(newRow, "vBCST", vBCST)) return false;
  if (not modelConsumo.setData(newRow, "vICMSST", vICMSST)) return false;
  if (not modelConsumo.setData(newRow, "vBCPIS", vBCPIS)) return false;
  if (not modelConsumo.setData(newRow, "vPIS", vPIS)) return false;
  if (not modelConsumo.setData(newRow, "vBCCOFINS", vBCCOFINS)) return false;
  if (not modelConsumo.setData(newRow, "vCOFINS", vCOFINS)) return false;

  if (not modelConsumo.submitAll()) {
    QMessageBox::critical(this, "Erro!", "Erro salvando dados: " + modelConsumo.lastError().text());
    return false;
  }

  return true;
}

bool Estoque::quebrarCompra(const int idVendaProduto, const double quant) {
  // se quant a consumir for igual a quant da compra apenas alterar idVenda/produto
  // senao fazer a quebra

  if (modelCompra.rowCount() == 0) {
    QMessageBox::critical(this, "Erro!", "Nenhuma linha de compra!");
    return false;
  }

  QSqlQuery query;
  query.prepare("SELECT idVenda FROM venda_has_produto WHERE idVendaProduto = :idVendaProduto");
  query.bindValue(":idVendaProduto", idVendaProduto);

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando idVenda: " + query.lastError().text());
    return false;
  }

  if (quant < modelCompra.data(0, "quant").toDouble()) {
    const int newRow = modelCompra.rowCount();
    modelCompra.insertRow(newRow);

    for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
      if (modelCompra.fieldIndex("idPedido") == column) continue;
      if (modelCompra.fieldIndex("created") == column) continue;
      if (modelCompra.fieldIndex("lastUpdated") == column) continue;

      const QVariant value = modelCompra.data(0, column);

      if (not modelCompra.setData(newRow, column, value)) return false;
    }

    const double caixas = modelCompra.data(0, "caixas").toDouble();
    const double prcUnitario = modelCompra.data(0, "prcUnitario").toDouble();
    const double quantConsumida = modelCompra.data(0, "quantConsumida").toDouble();
    const double quantOriginal = modelCompra.data(0, "quant").toDouble();
    const double proporcaoNovo = quant / quantOriginal;
    const double proporcaoAntigo = (quantOriginal - quant) / quantOriginal;

    if (not modelCompra.setData(newRow, "idVenda", query.value("idVenda"))) return false;
    if (not modelCompra.setData(newRow, "idVendaProduto", idVendaProduto)) return false;
    if (not modelCompra.setData(newRow, "quant", quant)) return false;
    if (not modelCompra.setData(newRow, "quantConsumida", qMin(quant, quantConsumida))) return false;
    if (not modelCompra.setData(newRow, "caixas", caixas * proporcaoNovo)) return false;
    if (not modelCompra.setData(newRow, "preco", prcUnitario * quant)) return false;

    if (not modelCompra.setData(0, "quant", quantOriginal - quant)) return false;
    if (not modelCompra.setData(0, "quantConsumida", qMin(quantOriginal - quant, quantConsumida))) return false;
    if (not modelCompra.setData(0, "caixas", caixas * proporcaoAntigo)) return false;
    if (not modelCompra.setData(0, "preco", prcUnitario * (quantOriginal - quant))) return false;

  } else {
    if (not modelCompra.setData(0, "idVenda", query.value("idVenda"))) return false;
    if (not modelCompra.setData(0, "idVendaProduto", idVendaProduto)) return false;
  }

  if (not modelCompra.submitAll()) {
    QMessageBox::critical(this, "Erro!", "Erro salvando dados da compra: " + modelCompra.lastError().text());
    return false;
  }

  return true;
}

void Estoque::on_tableEstoque_entered(const QModelIndex) { ui->tableEstoque->resizeColumnsToContents(); }

void Estoque::on_tableConsumo_entered(const QModelIndex) { ui->tableConsumo->resizeColumnsToContents(); }

// TODO: 1colocar o botao de desvincular consumo nesta tela
