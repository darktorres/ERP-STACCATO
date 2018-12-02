#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "estoqueproxymodel.h"
#include "ui_estoque.h"
#include "usersession.h"
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

  const QString tipoUsuario = UserSession::tipoUsuario();

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") { ui->pushButtonExibirNfe->hide(); }

  ui->labelRestante_2->hide();
  ui->doubleSpinBoxComprometidoNaoEntregue->hide();
}

Estoque::~Estoque() { delete ui; }

void Estoque::setupTables() {
  modelEstoque.setTable("estoque");
  modelEstoque.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelEstoque.setHeaderData("idEstoque", "Estoque");
  modelEstoque.setHeaderData("recebidoPor", "Recebido Por");
  modelEstoque.setHeaderData("status", "Status");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("fornecedor", "Fornecedor");
  modelEstoque.setHeaderData("descricao", "Produto");
  modelEstoque.setHeaderData("observacao", "Obs.");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Caixas");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("bloco", "Bloco");

  ui->tableEstoque->setModel(new EstoqueProxyModel(&modelEstoque, this));
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

  //--------------------------------------------------------------------

  modelConsumo.setTable("estoque_has_consumo");
  modelConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelConsumo.setFilter("0");

  //--------------------------------------------------------------------

  modelViewConsumo.setTable("view_estoque_consumo");
  modelViewConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelViewConsumo.setHeaderData("statusProduto", "Status Pedido");
  modelViewConsumo.setHeaderData("status", "Status Consumo");
  modelViewConsumo.setHeaderData("ordemCompra", "OC");
  modelViewConsumo.setHeaderData("local", "Local");
  modelViewConsumo.setHeaderData("fornecedor", "Fornecedor");
  modelViewConsumo.setHeaderData("descricao", "Produto");
  modelViewConsumo.setHeaderData("quant", "Quant.");
  modelViewConsumo.setHeaderData("un", "Un.");
  modelViewConsumo.setHeaderData("caixas", "Caixas");
  modelViewConsumo.setHeaderData("codComercial", "Cód. Com.");
  modelViewConsumo.setHeaderData("dataRealEnt", "Entrega");
  modelViewConsumo.setHeaderData("created", "Criado");

  ui->tableConsumo->setModel(new EstoqueProxyModel(&modelViewConsumo, this));
  ui->tableConsumo->setItemDelegateForColumn("quant", new DoubleDelegate(this, 4));
  ui->tableConsumo->showColumn("created");
  ui->tableConsumo->hideColumn("idEstoque");
  ui->tableConsumo->hideColumn("quantUpd");
}

void Estoque::on_tableEstoque_activated(const QModelIndex &) { exibirNota(); }

void Estoque::calcularRestante() {
  double quantRestante = modelEstoque.data(0, "quant").toDouble();
  double quantComprometidoNaoEntregue = modelEstoque.data(0, "quant").toDouble();

  for (int row = 0; row < modelViewConsumo.rowCount(); ++row) {
    const double quant = modelViewConsumo.data(row, "quant").toDouble();
    const QString statusProduto = modelViewConsumo.data(row, "statusProduto").toString();

    quantRestante += quant;

    if (statusProduto == "ENTREGUE") { quantComprometidoNaoEntregue -= quant * -1; }
  }

  const QString un = modelEstoque.data(0, "un").toString();

  ui->doubleSpinBoxRestante->setValue(quantRestante);
  ui->doubleSpinBoxRestante->setSuffix(" " + un);

  ui->doubleSpinBoxComprometidoNaoEntregue->setValue(quantComprometidoNaoEntregue);
  ui->doubleSpinBoxComprometidoNaoEntregue->setSuffix(" " + un);
}

bool Estoque::viewRegisterById(const bool showWindow) {
  if (idEstoque.isEmpty()) { return qApp->enqueueError(false, "Estoque não encontrado!"); }

  modelEstoque.setFilter("idEstoque = " + idEstoque);

  if (not modelEstoque.select()) { return false; }

  ui->tableEstoque->resizeColumnsToContents();

  if (not modelConsumo.select()) { return false; }

  modelViewConsumo.setFilter("idEstoque = " + idEstoque);

  if (not modelViewConsumo.select()) { return false; }

  ui->tableConsumo->resizeColumnsToContents();

  calcularRestante();

  if (showWindow) { show(); }

  return true;
}

void Estoque::on_pushButtonExibirNfe_clicked() { exibirNota(); }

void Estoque::exibirNota() {
  QSqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelEstoque.data(0, "idNFe"));

  if (not query.exec()) { return qApp->enqueueError("Erro buscando nfe: " + query.lastError().text()); }

  if (query.size() == 0) { return qApp->enqueueWarning("Não encontrou NFe associada!"); }

  while (query.next()) {
    auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
  }
}

bool Estoque::atualizaQuantEstoque() {
  // NOTE: can this be simplified so no query is necessary?
  QSqlQuery query;
  query.prepare("UPDATE produto p, view_estoque2 v SET p.estoqueRestante = v.restante, descontinuado = IF(v.restante = 0, TRUE, FALSE) WHERE p.idEstoque = v.idEstoque AND v.idEstoque = :idEstoque");
  query.bindValue(":idEstoque", modelEstoque.data(0, "idEstoque"));

  if (not query.exec()) { return qApp->enqueueError(false, "Erro atualizando quant. estoque: " + query.lastError().text()); }

  return true;
}

bool Estoque::criarConsumo(const int idVendaProduto, const double quant) {
  // TODO: verificar se as divisões de linha batem com a outra função criarConsumo

  if (modelEstoque.filter().isEmpty()) { return qApp->enqueueError(false, "Não setou idEstoque!"); }

  if (quant > ui->doubleSpinBoxRestante->value()) { return qApp->enqueueError(false, "Quantidade insuficiente!"); }

  // -------------------------------------------------------------------------

  const auto idPedido = dividirCompra(idVendaProduto, quant);

  // -------------------------------------------------------------------------

  const int rowEstoque = 0;
  const int rowConsumo = modelConsumo.rowCount();

  modelConsumo.insertRow(rowConsumo);

  for (int column = 0, columnCount = modelEstoque.columnCount(); column < columnCount; ++column) {
    const QString field = modelEstoque.record().fieldName(column);
    const int index = modelConsumo.fieldIndex(field);

    if (index == -1) { continue; }
    if (column == modelEstoque.fieldIndex("cfop")) { break; }

    const QVariant value = modelEstoque.data(rowEstoque, column);

    if (not modelConsumo.setData(rowConsumo, index, value)) { return false; }
  }

  QSqlQuery query;
  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelEstoque.data(rowEstoque, "idProduto"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + query.lastError().text()); }

  const QString un = query.value("un").toString();
  const double m2cx = query.value("m2cx").toDouble();
  const double pccx = query.value("pccx").toDouble();

  const double unCaixa = un == "M2" or un == "M²" or un == "ML" ? m2cx : pccx;

  const double caixas = qRound(quant / unCaixa * 100) / 100.;

  if (not modelConsumo.setData(rowConsumo, "quant", quant * -1)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "caixas", caixas)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "quantUpd", static_cast<int>(FieldColors::DarkGreen))) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idVendaProduto", idVendaProduto)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"))) { return false; }
  if (idPedido and not modelConsumo.setData(rowConsumo, "idPedido", idPedido.value())) { return false; }
  if (not modelConsumo.setData(rowConsumo, "status", "CONSUMO")) { return false; }

  if (not modelConsumo.submitAll()) { return false; }

  if (not atualizaQuantEstoque()) { return false; }

  // -------------------------------------------------------------------------
  // copy lote to venda_has_produto

  const QString lote = modelEstoque.data(rowEstoque, "lote").toString();

  if (not lote.isEmpty() and lote != "N/D") {
    QSqlQuery queryProduto;
    queryProduto.prepare("UPDATE venda_has_produto SET lote = :lote WHERE idVendaProduto = :idVendaProduto");
    queryProduto.bindValue(":lote", modelEstoque.data(rowEstoque, "lote"));
    queryProduto.bindValue(":idVendaProduto", idVendaProduto);

    if (not queryProduto.exec()) { return qApp->enqueueError(false, "Erro salvando lote: " + queryProduto.lastError().text()); }
  }

  // -------------------------------------------------------------------------

  return true;
}

std::optional<int> Estoque::dividirCompra(const int idVendaProduto, const double quant) {
  // se quant a consumir for igual a quant da compra apenas alterar idVenda/produto
  // senao fazer a quebra

  QSqlQuery query1;
  query1.prepare("SELECT pf.codComercial, pf.idCompra, pf.quant FROM estoque e LEFT JOIN estoque_has_compra ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN pedido_fornecedor_has_produto pf ON "
                 "pf.idPedido = ehc.idPedido WHERE e.idEstoque = :idEstoque");
  query1.bindValue(":idEstoque", idEstoque);

  if (not query1.exec() or not query1.first()) {
    qApp->enqueueError("Erro buscando compra relacionada ao estoque: " + query1.lastError().text());
    return {};
  }

  const QString idCompra = query1.value("idCompra").toString();
  const QString codComercial = query1.value("codComercial").toString();

  //--------------------------------------------------------------------

  SqlRelationalTableModel modelCompra;
  modelCompra.setTable("pedido_fornecedor_has_produto");
  modelCompra.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelCompra.setFilter("idCompra = " + idCompra + " AND codComercial = '" + codComercial + "' AND quant >= " + QString::number(quant) + " AND idVenda IS NULL AND idVendaProduto IS NULL");

  if (not modelCompra.select()) { return {}; }

  if (modelCompra.rowCount() == 0) { return {}; }

  QSqlQuery query;
  query.prepare("SELECT idVenda FROM venda_has_produto WHERE idVendaProduto = :idVendaProduto");
  query.bindValue(":idVendaProduto", idVendaProduto);

  if (not query.exec() or not query.first()) {
    qApp->enqueueError("Erro buscando idVenda: " + query.lastError().text());
    return {};
  }

  const int row = 0;
  const double quantCompra = modelCompra.data(row, "quant").toDouble();

  if (quant > quantCompra) {
    qApp->enqueueError("Erro quant > quantCompra");
    return {};
  }

  if (qFuzzyCompare(quant, quantCompra)) {
    if (not modelCompra.setData(row, "idVenda", query.value("idVenda"))) { return {}; }
    if (not modelCompra.setData(row, "idVendaProduto", idVendaProduto)) { return {}; }
  }

  const bool dividir = quant < quantCompra;

  if (dividir) {
    const int newRow = modelCompra.rowCount();
    // NOTE: *quebralinha venda_produto/pedido_fornecedor
    modelCompra.insertRow(newRow);

    for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
      if (modelCompra.fieldIndex("idPedido") == column) { continue; }
      if (modelCompra.fieldIndex("created") == column) { continue; }
      if (modelCompra.fieldIndex("lastUpdated") == column) { continue; }

      const QVariant value = modelCompra.data(row, column);

      if (not modelCompra.setData(newRow, column, value)) { return {}; }
    }

    const double caixas = modelCompra.data(row, "caixas").toDouble();
    const double prcUnitario = modelCompra.data(row, "prcUnitario").toDouble();
    const double quantConsumida = modelCompra.data(row, "quantConsumida").toDouble();
    const double quantOriginal = modelCompra.data(row, "quant").toDouble();
    const double proporcaoNovo = quant / quantOriginal;
    const double proporcaoAntigo = (quantOriginal - quant) / quantOriginal;

    if (not modelCompra.setData(newRow, "idVenda", query.value("idVenda"))) { return {}; }
    if (not modelCompra.setData(newRow, "idVendaProduto", idVendaProduto)) { return {}; }
    if (not modelCompra.setData(newRow, "quant", quant)) { return {}; }
    if (not modelCompra.setData(newRow, "quantConsumida", qMin(quant, quantConsumida))) { return {}; }
    if (not modelCompra.setData(newRow, "caixas", caixas * proporcaoNovo)) { return {}; }
    if (not modelCompra.setData(newRow, "preco", prcUnitario * quant)) { return {}; }

    if (not modelCompra.setData(row, "quant", quantOriginal - quant)) { return {}; }
    if (not modelCompra.setData(row, "quantConsumida", qMin(quantOriginal - quant, quantConsumida))) { return {}; }
    if (not modelCompra.setData(row, "caixas", caixas * proporcaoAntigo)) { return {}; }
    if (not modelCompra.setData(row, "preco", prcUnitario * (quantOriginal - quant))) { return {}; }
  }

  const int id = modelCompra.data(row, "idPedido").toInt();

  if (not modelCompra.submitAll()) { return {}; }

  return dividir ? modelCompra.query().lastInsertId().toInt() : id;
}

void Estoque::on_tableEstoque_entered(const QModelIndex) { ui->tableEstoque->resizeColumnsToContents(); }

void Estoque::on_tableConsumo_entered(const QModelIndex) { ui->tableConsumo->resizeColumnsToContents(); }

bool Estoque::desfazerConsumo() {
  // there is one implementation in InputDialogConfirmacao
  // and another one in WidgetCompraOC
  // TODO: juntar as lógicas
  // TODO: se houver agendamento de estoque remover

  return true;
}

// TODO: 1colocar o botao de desvincular consumo nesta tela
// TODO: no view_widget_estoque deixar apenas o status do consumo
