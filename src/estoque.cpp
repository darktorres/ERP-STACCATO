#include "estoque.h"
#include "ui_estoque.h"

#include "application.h"
#include "doubledelegate.h"
#include "estoqueproxymodel.h"
#include "usersession.h"
#include "xml_viewer.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

Estoque::Estoque(const QString &idEstoque, const bool showWindow, QWidget *parent) : QDialog(parent), idEstoque(idEstoque), ui(new Ui::Estoque) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  connect(ui->pushButtonExibirNfe, &QPushButton::clicked, this, &Estoque::on_pushButtonExibirNfe_clicked);

  viewRegisterById(showWindow);

  const QString tipoUsuario = UserSession::tipoUsuario;

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") { ui->pushButtonExibirNfe->hide(); }
}

Estoque::~Estoque() { delete ui; }

void Estoque::setupTables() {
  modelEstoque.setTable("estoque");

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

  modelEstoque.proxyModel = new EstoqueProxyModel(&modelEstoque, this);

  ui->tableEstoque->setModel(&modelEstoque);

  ui->tableEstoque->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));

  ui->tableEstoque->hideColumn("restante");
  ui->tableEstoque->hideColumn("ajuste");
  ui->tableEstoque->hideColumn("idNFe");
  ui->tableEstoque->hideColumn("quantUpd");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("codBarras");
  ui->tableEstoque->hideColumn("ncm");
  ui->tableEstoque->hideColumn("nve");
  ui->tableEstoque->hideColumn("extipi");
  ui->tableEstoque->hideColumn("cest");
  ui->tableEstoque->hideColumn("cfop");
  ui->tableEstoque->hideColumn("valorUnid");
  ui->tableEstoque->hideColumn("valor");
  ui->tableEstoque->hideColumn("codBarrasTrib");
  ui->tableEstoque->hideColumn("unTrib");
  ui->tableEstoque->hideColumn("quantTrib");
  ui->tableEstoque->hideColumn("valorUnidTrib");
  ui->tableEstoque->hideColumn("frete");
  ui->tableEstoque->hideColumn("seguro");
  ui->tableEstoque->hideColumn("desconto");
  ui->tableEstoque->hideColumn("outros");
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
  ui->tableEstoque->hideColumn("vBCIPI");
  ui->tableEstoque->hideColumn("pIPI");
  ui->tableEstoque->hideColumn("vIPI");
  ui->tableEstoque->hideColumn("cstPIS");
  ui->tableEstoque->hideColumn("vBCPIS");
  ui->tableEstoque->hideColumn("pPIS");
  ui->tableEstoque->hideColumn("vPIS");
  ui->tableEstoque->hideColumn("cstCOFINS");
  ui->tableEstoque->hideColumn("vBCCOFINS");
  ui->tableEstoque->hideColumn("pCOFINS");
  ui->tableEstoque->hideColumn("vCOFINS");
  ui->tableEstoque->hideColumn("valorGare");

  //--------------------------------------------------------------------

  modelConsumo.setTable("estoque_has_consumo");

  //--------------------------------------------------------------------

  modelViewConsumo.setTable("view_estoque_consumo");

  modelViewConsumo.setHeaderData("statusProduto", "Status Pedido");
  modelViewConsumo.setHeaderData("status", "Status Consumo");
  modelViewConsumo.setHeaderData("bloco", "Bloco");
  modelViewConsumo.setHeaderData("fornecedor", "Fornecedor");
  modelViewConsumo.setHeaderData("descricao", "Produto");
  modelViewConsumo.setHeaderData("quant", "Quant.");
  modelViewConsumo.setHeaderData("un", "Un.");
  modelViewConsumo.setHeaderData("caixas", "Caixas");
  modelViewConsumo.setHeaderData("codComercial", "Cód. Com.");
  modelViewConsumo.setHeaderData("dataRealEnt", "Entrega");
  modelViewConsumo.setHeaderData("created", "Criado");

  modelViewConsumo.proxyModel = new EstoqueProxyModel(&modelViewConsumo, this);

  ui->tableConsumo->setModel(&modelViewConsumo);

  ui->tableConsumo->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));

  ui->tableConsumo->showColumn("created");
  ui->tableConsumo->hideColumn("idEstoque");
  ui->tableConsumo->hideColumn("quantUpd");
}

void Estoque::buscarRestante() {
  const double quantRestante = modelEstoque.data(0, "restante").toDouble();
  const QString un = modelEstoque.data(0, "un").toString();

  ui->doubleSpinBoxRestante->setValue(quantRestante);
  ui->doubleSpinBoxRestante->setSuffix(" " + un);
}

bool Estoque::viewRegisterById(const bool showWindow) {
  if (idEstoque.isEmpty()) { throw RuntimeException("Estoque não encontrado!", this); }

  modelEstoque.setFilter("idEstoque = " + idEstoque);

  modelEstoque.select();

  //--------------------------------------

  modelViewConsumo.setFilter("idEstoque = " + idEstoque);

  modelViewConsumo.select();

  //--------------------------------------

  buscarRestante();

  if (showWindow) { show(); }

  return true;
}

void Estoque::on_pushButtonExibirNfe_clicked() { exibirNota(); }

void Estoque::exibirNota() {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelEstoque.data(0, "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro buscando NFe: " + query.lastError().text(), this); }

  if (query.size() == 0) { return qApp->enqueueWarning("Não encontrou NFe associada!", this); }

  while (query.next()) {
    auto *viewer = new XML_Viewer(query.value("xml").toByteArray(), this);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
  }
}

void Estoque::criarConsumo(const int idVendaProduto2, const double quant) {
  // TODO: verificar se as divisões de linha batem com a outra função criarConsumo

  if (modelEstoque.filter().isEmpty()) { throw RuntimeException("Não setou idEstoque!"); }

  if (quant > ui->doubleSpinBoxRestante->value()) { throw RuntimeException("Quantidade insuficiente!"); }

  // -------------------------------------------------------------------------

  dividirCompra(idVendaProduto2, quant);

  // -------------------------------------------------------------------------

  const int rowEstoque = 0;
  const int rowConsumo = modelConsumo.insertRowAtEnd();

  for (int column = 0, columnCount = modelEstoque.columnCount(); column < columnCount; ++column) {
    const QString field = modelEstoque.record().fieldName(column);
    const int index = modelConsumo.fieldIndex(field, true);

    if (index == -1) { continue; }
    if (column == modelEstoque.fieldIndex("valor")) { break; }

    const QVariant value = modelEstoque.data(rowEstoque, column);

    if (value.isNull()) { continue; }

    modelConsumo.setData(rowConsumo, index, value);
  }

  SqlQuery query;
  query.prepare("SELECT quantCaixa FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelEstoque.data(rowEstoque, "idProduto"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando dados do produto: " + query.lastError().text()); }

  const double quantCaixa = query.value("quantCaixa").toDouble();
  const double caixas = quant / quantCaixa;

  modelConsumo.setData(rowConsumo, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"));
  modelConsumo.setData(rowConsumo, "idVendaProduto2", idVendaProduto2);
  modelConsumo.setData(rowConsumo, "status", "CONSUMO");
  modelConsumo.setData(rowConsumo, "quant", quant * -1);
  modelConsumo.setData(rowConsumo, "quantUpd", static_cast<int>(FieldColors::DarkGreen));
  modelConsumo.setData(rowConsumo, "caixas", caixas);
  const double valorUnid = modelConsumo.data(rowConsumo, "valorUnid").toDouble();
  modelConsumo.setData(rowConsumo, "valor", quant * valorUnid);

  modelConsumo.submitAll();

  // -------------------------------------------------------------------------
  // copy lote to venda_has_produto

  const QString lote = modelEstoque.data(rowEstoque, "lote").toString();

  if (not lote.isEmpty() and lote != "N/D") {
    SqlQuery queryProduto;
    queryProduto.prepare("UPDATE venda_has_produto2 SET lote = :lote WHERE idVendaProduto2 = :idVendaProduto2");
    queryProduto.bindValue(":lote", modelEstoque.data(rowEstoque, "lote"));
    queryProduto.bindValue(":idVendaProduto2", idVendaProduto2);

    if (not queryProduto.exec()) { throw RuntimeException("Erro salvando lote: " + queryProduto.lastError().text()); }
  }
}

void Estoque::dividirCompra(const int idVendaProduto2, const double quant) {
  // se quant a consumir for igual a quant da compra apenas alterar idVenda/produto
  // senao fazer a quebra

  SqlTableModel modelCompra;
  modelCompra.setTable("pedido_fornecedor_has_produto2");

  const QString subQuery = "SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = " + idEstoque;
  modelCompra.setFilter("(idPedido2 IN (" + subQuery + ") OR idRelacionado IN (" + subQuery + ")) AND idVenda IS NULL AND idVendaProduto2 IS NULL AND quant >= " + QString::number(quant));

  modelCompra.select();

  if (modelCompra.rowCount() == 0) { return; }

  //--------------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT idVenda FROM venda_has_produto2 WHERE idVendaProduto2 = :idVendaProduto2");
  query.bindValue(":idVendaProduto2", idVendaProduto2);

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando idVenda: " + query.lastError().text()); }

  const int row = 0;
  const double quantCompra = modelCompra.data(row, "quant").toDouble();

  if (quant > quantCompra) { throw RuntimeException("Erro quant > quantCompra"); }

  if (qFuzzyCompare(quant, quantCompra)) {
    modelCompra.setData(row, "idVenda", query.value("idVenda"));
    modelCompra.setData(row, "idVendaProduto2", idVendaProduto2);
  }

  const bool dividir = quant < quantCompra;

  if (dividir) {
    // NOTE: *quebralinha pedido_fornecedor2

    const double caixas = modelCompra.data(row, "caixas").toDouble();
    const double prcUnitario = modelCompra.data(row, "prcUnitario").toDouble();
    const double quantOriginal = modelCompra.data(row, "quant").toDouble();
    const double proporcaoNovo = quant / quantOriginal;
    const double proporcaoAntigo = (quantOriginal - quant) / quantOriginal;

    modelCompra.setData(row, "quant", quantOriginal - quant);
    modelCompra.setData(row, "caixas", caixas * proporcaoAntigo);
    modelCompra.setData(row, "preco", prcUnitario * (quantOriginal - quant));

    // -------------------------------------------------------------------------

    const int newRow = modelCompra.insertRowAtEnd();

    for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
      if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
      if (column == modelCompra.fieldIndex("created")) { continue; }
      if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

      const QVariant value = modelCompra.data(row, column);

      if (value.isNull()) { continue; }

      modelCompra.setData(newRow, column, value);
    }

    // -------------------------------------------------------------------------

    modelCompra.setData(newRow, "idRelacionado", modelCompra.data(row, "idPedido2"));
    modelCompra.setData(newRow, "idVenda", query.value("idVenda"));
    modelCompra.setData(newRow, "idVendaProduto2", idVendaProduto2);
    modelCompra.setData(newRow, "quant", quant);
    modelCompra.setData(newRow, "caixas", caixas * proporcaoNovo);
    modelCompra.setData(newRow, "preco", prcUnitario * quant);
  }

  modelCompra.submitAll();
}

void Estoque::desfazerConsumo(const int idVendaProduto2) {
  // there is one implementation in InputDialogConfirmacao
  // TODO: juntar as lógicas
  // TODO: se houver agendamento de estoque remover

  // NOTE: estoque_has_consumo may have the same idVendaProduto2 in more than one row (only until the field is made UNIQUE)
  SqlQuery queryDelete;
  queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idVendaProduto2 = :idVendaProduto2");
  queryDelete.bindValue(":idVendaProduto2", idVendaProduto2);

  if (not queryDelete.exec()) { throw RuntimeException("Erro removendo consumo estoque: " + queryDelete.lastError().text()); }

  // TODO: juntar linhas sem consumo do mesmo tipo? (usar idRelacionado)
  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, idVendaProduto2 = NULL WHERE idVendaProduto2 = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");
  queryCompra.bindValue(":idVendaProduto2", idVendaProduto2);

  if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando pedido compra: " + queryCompra.lastError().text()); }

  SqlQuery queryVenda;
  queryVenda.prepare(
      "UPDATE venda_has_produto2 SET status = CASE WHEN reposicaoEntrega THEN 'REPO. ENTREGA' WHEN reposicaoReceb THEN 'REPO. RECEB.' ELSE 'PENDENTE' END, idCompra = NULL, lote = NULL, "
      "dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, dataPrevReceb = "
      "NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idVendaProduto2` = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  queryVenda.bindValue(":idVendaProduto2", idVendaProduto2);

  if (not queryVenda.exec()) { throw RuntimeException("Erro atualizando pedido venda: " + queryVenda.lastError().text()); }
}

// TODO: 1colocar o botao de desvincular consumo nesta tela
// TODO: no view_widget_estoque deixar apenas o status do consumo
