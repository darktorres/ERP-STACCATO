#include "produtospendentes.h"
#include "ui_produtospendentes.h"

#include "application.h"
#include "doubledelegate.h"
#include "editdelegate.h"
#include "estoque.h"
#include "inputdialog.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

ProdutosPendentes::ProdutosPendentes(QWidget *parent) : QDialog(parent), ui(new Ui::ProdutosPendentes) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);
  setupTables();
  setConnections();
}

ProdutosPendentes::~ProdutosPendentes() { delete ui; }

void ProdutosPendentes::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonComprar, &QPushButton::clicked, this, &ProdutosPendentes::on_pushButtonComprar_clicked, connectionType);
  connect(ui->pushButtonConsumirCompra, &QPushButton::clicked, this, &ProdutosPendentes::on_pushButtonConsumirCompra_clicked, connectionType);
  connect(ui->pushButtonConsumirEstoque, &QPushButton::clicked, this, &ProdutosPendentes::on_pushButtonConsumirEstoque_clicked, connectionType);
}

void ProdutosPendentes::recalcularQuantidade() {
  const auto selectedRows = ui->tableProdutos->selectionModel()->selectedRows();
  double quant = 0;

  for (const auto &index : selectedRows) { quant += modelViewProdutos.data(index.row(), "quant").toDouble(); }

  ui->doubleSpinBoxQuantTotal->setValue(quant);
  ui->doubleSpinBoxComprar->setValue(quant);
}

void ProdutosPendentes::viewProduto(const QString &fornecedor, const QString &codComercial, const QString &idVenda) {
  setWindowTitle("Venda " + idVenda);

  //-----------------------------------------------

  modelProdutos.setFilter("fornecedor = '" + fornecedor + "' AND codComercial = '" + codComercial + "' AND idVenda = '" + idVenda + "' AND status IN ('PENDENTE', 'REPO. ENTREGA', 'REPO. RECEB.')");

  modelProdutos.select();

  //-----------------------------------------------

  modelViewProdutos.setFilter("fornecedor = '" + fornecedor + "' AND codComercial = '" + codComercial + "' AND idVenda = '" + idVenda +
                              "' AND status IN ('PENDENTE', 'REPO. ENTREGA', 'REPO. RECEB.')");

  modelViewProdutos.select();

  //-----------------------------------------------

  modelCompra.setFilter("fornecedor = '" + fornecedor + "' AND codComercial = '" + codComercial + "' AND idVendaProduto2 IS NULL AND status IN ('PENDENTE', 'EM COMPRA', 'EM FATURAMENTO')");

  modelCompra.select();

  //-----------------------------------------------

  ui->doubleSpinBoxQuantTotal->setSuffix(" " + modelViewProdutos.data(0, "un").toString());
  ui->doubleSpinBoxComprar->setSuffix(" " + modelViewProdutos.data(0, "un").toString());

  //-----------------------------------------------

  SqlQuery query;
  query.prepare("SELECT quantCaixa FROM venda_has_produto2 WHERE `idVendaProduto2` = :idVendaProduto2");
  query.bindValue(":idVendaProduto2", modelViewProdutos.data(0, "idVendaProduto2"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando quantCaixa: " + query.lastError().text(), this); }

  const double step = query.value("quantCaixa").toDouble();

  ui->doubleSpinBoxQuantTotal->setSingleStep(step);
  ui->doubleSpinBoxComprar->setSingleStep(step);

  //-----------------------------------------------

  // TODO: move query to Sql class
  modelEstoque.setQuery(
      "SELECT `e`.`status` AS `status`, `e`.`idEstoque` AS `idEstoque`, `e`.`descricao` AS `descricao`, e.restante, `e`.`un` AS `unEst`, e.restante / p.quantCaixa AS `Caixas`, `e`.`lote` AS `lote`, "
      "`e`.`local` AS `local`, `e`.`bloco` AS `bloco`, `e`.`codComercial` AS `codComercial` FROM `estoque` `e` LEFT JOIN `produto` `p` ON `e`.`idProduto` = `p`.`idProduto` WHERE e.status NOT IN "
      "('CANCELADO' , 'IGNORAR') AND p.fornecedor = '" +
      fornecedor + "' AND e.codComercial = '" + codComercial + "' GROUP BY `e`.`idEstoque` HAVING restante > 0");

  modelEstoque.select();

  modelEstoque.setHeaderData("status", "Status");
  modelEstoque.setHeaderData("idEstoque", "Estoque");
  modelEstoque.setHeaderData("descricao", "Descrição");
  modelEstoque.setHeaderData("restante", "Disponível");
  modelEstoque.setHeaderData("unEst", "Un.");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("bloco", "Bloco");

  ui->tableEstoque->setModel(&modelEstoque);
  ui->tableEstoque->setItemDelegateForColumn("restante", new DoubleDelegate(3, this));

  //-----------------------------------------------

  const bool representacao = (idVenda.at(11) == 'R');

  representacao ? ui->tableProdutos->hideColumn("custo") : ui->tableProdutos->hideColumn("custoVenda");

  //-----------------------------------------------

  if (modelViewProdutos.rowCount() == 1) { ui->tableProdutos->selectRow(0); }
  if (modelEstoque.rowCount() == 1) { ui->tableEstoque->selectRow(0); }
}

void ProdutosPendentes::setupTables() {
  modelProdutos.setTable("venda_has_produto2");

  //--------------------------------------------------------------------

  modelViewProdutos.setTable("view_produto_pendente");

  modelViewProdutos.setHeaderData("status", "Status");
  modelViewProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelViewProdutos.setHeaderData("produto", "Descrição");
  modelViewProdutos.setHeaderData("colecao", "Coleção");
  modelViewProdutos.setHeaderData("formComercial", "Form. Com.");
  modelViewProdutos.setHeaderData("caixas", "Caixas");
  modelViewProdutos.setHeaderData("kgcx", "Kg./Cx.");
  modelViewProdutos.setHeaderData("quant", "Quant.");
  modelViewProdutos.setHeaderData("un", "Un.");
  modelViewProdutos.setHeaderData("un2", "Un.2");
  modelViewProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelViewProdutos.setHeaderData("codBarras", "Cód. Barras");
  modelViewProdutos.setHeaderData("custo", "Custo");
  modelViewProdutos.setHeaderData("custoVenda", "Custo Rep.");
  modelViewProdutos.setHeaderData("obs", "Obs.");

  ui->tableProdutos->setModel(&modelViewProdutos);

  ui->tableProdutos->setItemDelegate(new NoEditDelegate(this));

  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(3, this));
  ui->tableProdutos->setItemDelegateForColumn("custo", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("custoVenda", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("obs", new EditDelegate(this));

  ui->tableProdutos->hideColumn("idVenda");
  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("idVendaProdutoFK");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idCompra");
  ui->tableProdutos->hideColumn("st");

  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ProdutosPendentes::recalcularQuantidade);

  //--------------------------------------------------------------------

  modelCompra.setTable("pedido_fornecedor_has_produto2");

  modelCompra.setHeaderData("status", "Status");
  modelCompra.setHeaderData("ordemCompra", "OC");
  modelCompra.setHeaderData("descricao", "Descrição");
  modelCompra.setHeaderData("obs", "Obs.");
  modelCompra.setHeaderData("quant", "Quant.");
  modelCompra.setHeaderData("un", "Un.");
  modelCompra.setHeaderData("caixas", "Caixas");
  modelCompra.setHeaderData("dataPrevCompra", "Prev. Compra");
  modelCompra.setHeaderData("dataRealCompra", "Data Compra");
  modelCompra.setHeaderData("dataPrevConf", "Prev. Confirm.");
  modelCompra.setHeaderData("dataRealConf", "Data Confirm.");
  modelCompra.setHeaderData("dataPrevFat", "Prev. Fat.");

  ui->tableCompra->setModel(&modelCompra);

  ui->tableCompra->setItemDelegate(new NoEditDelegate(this));

  ui->tableCompra->hideColumn("idPedido2");
  ui->tableCompra->hideColumn("idPedidoFK");
  ui->tableCompra->hideColumn("idRelacionado");
  ui->tableCompra->hideColumn("selecionado");
  ui->tableCompra->hideColumn("aliquotaSt");
  ui->tableCompra->hideColumn("st");
  ui->tableCompra->hideColumn("statusFinanceiro");
  ui->tableCompra->hideColumn("ordemRepresentacao");
  ui->tableCompra->hideColumn("codFornecedor");
  ui->tableCompra->hideColumn("idVenda");
  ui->tableCompra->hideColumn("idVendaProduto1");
  ui->tableCompra->hideColumn("idVendaProduto2");
  ui->tableCompra->hideColumn("idCompra");
  ui->tableCompra->hideColumn("fornecedor");
  ui->tableCompra->hideColumn("idProduto");
  ui->tableCompra->hideColumn("colecao");
  ui->tableCompra->hideColumn("codComercial");
  ui->tableCompra->hideColumn("quantUpd");
  ui->tableCompra->hideColumn("un2");
  ui->tableCompra->hideColumn("prcUnitario");
  ui->tableCompra->hideColumn("preco");
  ui->tableCompra->hideColumn("kgcx");
  ui->tableCompra->hideColumn("formComercial");
  ui->tableCompra->hideColumn("codBarras");
  ui->tableCompra->hideColumn("dataRealFat");
  ui->tableCompra->hideColumn("dataPrevColeta");
  ui->tableCompra->hideColumn("dataRealColeta");
  ui->tableCompra->hideColumn("dataPrevReceb");
  ui->tableCompra->hideColumn("dataRealReceb");
  ui->tableCompra->hideColumn("dataPrevEnt");
  ui->tableCompra->hideColumn("dataRealEnt");
}

void ProdutosPendentes::comprar(const QModelIndexList &list, const QDate &dataPrevista) {
  for (const auto &index : list) {
    const int row = index.row();

    atualizarVenda(row);
    enviarProdutoParaCompra(row, dataPrevista);
    enviarExcedenteParaCompra(row, dataPrevista);
  }

  modelProdutos.submitAll();
}

void ProdutosPendentes::recarregarTabelas() {
  modelProdutos.select();
  modelViewProdutos.select();
  modelEstoque.select();

  ui->tableProdutos->clearSelection();

  if (modelViewProdutos.rowCount() == 1) { ui->tableProdutos->selectRow(0); }
  if (modelViewProdutos.rowCount() == 0) { close(); }
}

void ProdutosPendentes::on_pushButtonComprar_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum produto selecionado!", this); }

  if (list.size() > 1 and ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) { throw RuntimeError("Para comprar menos que o total selecione uma linha por vez!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxComprar->value())) { throw RuntimeError("Quantidade 0!", this); }

  const QString idVenda = modelViewProdutos.data(list.first().row(), "idVenda").toString();

  InputDialog inputDlg(InputDialog::Tipo::Carrinho, this);
  if (inputDlg.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("ProdutosPendentes::on_pushButtonComprar");

  comprar(list, inputDlg.getNextDate());

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  qApp->enqueueInformation("Produto enviado para carrinho!", this);

  recarregarTabelas();
}

void ProdutosPendentes::consumirCompra(const int rowProduto, const int rowCompra, const double quantConsumir, const double quantVenda) {
  const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();

  if (quantConsumir < quantVenda) { dividirVenda(quantConsumir, quantVenda, rowProduto); }
  if (quantConsumir < quantCompra) { dividirCompra(quantConsumir, quantCompra, rowCompra); }

  //--------------------------------------------------------------------

  const QString statusCompra = modelCompra.data(rowCompra, "status").toString();

  modelProdutos.setData(rowProduto, "status", (statusCompra == "PENDENTE" ? "INICIADO" : statusCompra));
  modelProdutos.setData(rowProduto, "dataPrevCompra", modelCompra.data(rowCompra, "dataPrevCompra"));
  modelProdutos.setData(rowProduto, "dataRealCompra", modelCompra.data(rowCompra, "dataRealCompra"));
  modelProdutos.setData(rowProduto, "dataPrevConf", modelCompra.data(rowCompra, "dataPrevConf"));
  modelProdutos.setData(rowProduto, "dataRealConf", modelCompra.data(rowCompra, "dataRealConf"));
  modelProdutos.setData(rowProduto, "dataPrevFat", modelCompra.data(rowCompra, "dataPrevFat"));

  //--------------------------------------------------------------------

  modelCompra.setData(rowCompra, "idVenda", modelProdutos.data(rowProduto, "idVenda"));
  modelCompra.setData(rowCompra, "idVendaProduto2", modelProdutos.data(rowProduto, "idVendaProduto2"));

  //--------------------------------------------------------------------

  modelProdutos.submitAll();
  modelCompra.submitAll();
}

void ProdutosPendentes::on_pushButtonConsumirCompra_clicked() {
  const auto listProduto = ui->tableProdutos->selectionModel()->selectedRows();

  if (listProduto.size() > 1) { throw RuntimeError("Selecione apenas um item na tabela de produtos!", this); }

  if (listProduto.isEmpty()) { throw RuntimeError("Nenhum produto selecionado!", this); }

  const auto listCompra = ui->tableCompra->selectionModel()->selectedRows();

  if (listCompra.isEmpty()) { throw RuntimeError("Nenhuma compra selecionada!", this); }

  //--------------------------------------------------------------------

  const int rowProduto = listProduto.first().row();
  const int rowCompra = listCompra.first().row();

  const double quantVenda = modelViewProdutos.data(rowProduto, "quant").toDouble();
  const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();

  bool ok;

  const double quantConsumir =
      QInputDialog::getDouble(this, "Consumo", "Quantidade a consumir: ", quantVenda, 0, qMin(quantVenda, quantCompra), 3, &ok, Qt::WindowFlags(), ui->doubleSpinBoxComprar->singleStep());

  if (not ok) { return; }

  const QString idVenda = modelViewProdutos.data(rowProduto, "idVenda").toString();

  //--------------------------------------------------------------------

  qApp->startTransaction("ProdutosPendentes::on_pushButtonConsumirCompra");

  consumirCompra(rowProduto, rowCompra, quantConsumir, quantVenda);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  //--------------------------------------------------------------------

  qApp->enqueueInformation("Consumo criado com sucesso!", this);

  recarregarTabelas();
}

void ProdutosPendentes::consumirEstoque(const int rowProduto, const int rowEstoque, const double quantConsumir, const double quantVenda) {
  auto *estoque = new Estoque(modelEstoque.data(rowEstoque, "idEstoque").toString(), false, this);
  estoque->criarConsumo(modelViewProdutos.data(rowProduto, "idVendaProduto2").toInt(), quantConsumir);

  //--------------------------------------------------------------------

  if (quantConsumir < quantVenda) { dividirVenda(quantConsumir, quantVenda, rowProduto); }

  //--------------------------------------------------------------------

  modelProdutos.setData(rowProduto, "status", modelEstoque.data(rowEstoque, "status"));

  modelProdutos.submitAll();
}

void ProdutosPendentes::on_pushButtonConsumirEstoque_clicked() {
  const auto listProduto = ui->tableProdutos->selectionModel()->selectedRows();

  if (listProduto.size() > 1) { throw RuntimeError("Selecione apenas um item na tabela de produtos!", this); }

  if (listProduto.isEmpty()) { throw RuntimeError("Nenhum produto selecionado!", this); }

  const auto listEstoque = ui->tableEstoque->selectionModel()->selectedRows();

  if (listEstoque.isEmpty()) { throw RuntimeError("Nenhum estoque selecionado!", this); }

  //--------------------------------------------------------------------

  const int rowProduto = listProduto.first().row();
  const int rowEstoque = listEstoque.first().row();

  const double quantVenda = modelViewProdutos.data(rowProduto, "quant").toDouble();
  const double quantEstoque = modelEstoque.data(rowEstoque, "restante").toDouble();

  bool ok;

  const double quantConsumir =
      QInputDialog::getDouble(this, "Consumo", "Quantidade a consumir: ", quantVenda, 0, qMin(quantVenda, quantEstoque), 3, &ok, Qt::WindowFlags(), ui->doubleSpinBoxComprar->singleStep());

  if (not ok) { return; }

  const QString idVenda = modelViewProdutos.data(rowProduto, "idVenda").toString();

  //--------------------------------------------------------------------

  qApp->startTransaction("ProdutosPendentes::on_pushButtonConsumirEstoque");

  consumirEstoque(rowProduto, rowEstoque, quantConsumir, quantVenda);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  //--------------------------------------------------------------------

  qApp->enqueueInformation("Consumo criado com sucesso!", this);

  recarregarTabelas();
}

void ProdutosPendentes::enviarExcedenteParaCompra(const int row, const QDate &dataPrevista) {
  const double excedente = ui->doubleSpinBoxComprar->value() - ui->doubleSpinBoxQuantTotal->value();

  if (excedente > 0.) { // TODO: replace query with model so values can be correctly rounded (Application::roundDouble)
    SqlQuery query;
    query.prepare("INSERT INTO pedido_fornecedor_has_produto (fornecedor, idProduto, descricao, colecao, quant, un, un2, caixas, prcUnitario, preco, kgcx, formComercial, codComercial, codBarras, "
                  "dataPrevCompra) VALUES (:fornecedor, :idProduto, :descricao, :colecao, :quant, :un, :un2, :caixas, :prcUnitario, :preco, :kgcx, :formComercial, :codComercial, :codBarras, "
                  ":dataPrevCompra)");
    query.bindValue(":fornecedor", modelViewProdutos.data(row, "fornecedor"));
    query.bindValue(":idProduto", modelViewProdutos.data(row, "idProduto"));
    query.bindValue(":descricao", modelViewProdutos.data(row, "produto"));
    query.bindValue(":colecao", modelViewProdutos.data(row, "colecao"));
    query.bindValue(":quant", excedente);
    query.bindValue(":un", modelViewProdutos.data(row, "un"));
    query.bindValue(":un2", modelViewProdutos.data(row, "un2"));
    query.bindValue(":caixas", excedente / modelProdutos.data(row, "quantCaixa").toDouble());
    query.bindValue(":prcUnitario", modelViewProdutos.data(row, "custo"));
    query.bindValue(":preco", modelViewProdutos.data(row, "custo").toDouble() * excedente);
    query.bindValue(":kgcx", modelViewProdutos.data(row, "kgcx"));
    query.bindValue(":formComercial", modelViewProdutos.data(row, "formComercial"));
    query.bindValue(":codComercial", modelViewProdutos.data(row, "codComercial"));
    query.bindValue(":codBarras", modelViewProdutos.data(row, "codBarras"));
    query.bindValue(":dataPrevCompra", dataPrevista);

    if (not query.exec()) { throw RuntimeException("Erro inserindo dados em pedido_fornecedor_has_produto: " + query.lastError().text()); }
  }
}

void ProdutosPendentes::enviarProdutoParaCompra(const int row, const QDate &dataPrevista) {
  SqlTableModel model;
  model.setTable("pedido_fornecedor_has_produto");

  const int newRow = model.insertRowAtEnd();

  const bool representacao = (modelViewProdutos.data(row, "idVenda").toString().at(11) == 'R');

  const double quant = (ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) ? ui->doubleSpinBoxComprar->value() : modelViewProdutos.data(row, "quant").toDouble();
  const double custo = representacao ? modelViewProdutos.data(row, "custoVenda").toDouble() : modelViewProdutos.data(row, "custo").toDouble();
  const double step = ui->doubleSpinBoxQuantTotal->singleStep();

  model.setData(newRow, "idVenda", modelViewProdutos.data(row, "idVenda"));
  model.setData(newRow, "idVendaProduto1", modelViewProdutos.data(row, "idVendaProdutoFK"));
  model.setData(newRow, "idVendaProduto2", modelViewProdutos.data(row, "idVendaProduto2"));
  model.setData(newRow, "fornecedor", modelViewProdutos.data(row, "fornecedor"));
  model.setData(newRow, "idProduto", modelViewProdutos.data(row, "idProduto"));
  model.setData(newRow, "descricao", modelViewProdutos.data(row, "produto"));
  model.setData(newRow, "obs", modelViewProdutos.data(row, "obs"));
  model.setData(newRow, "colecao", modelViewProdutos.data(row, "colecao"));
  model.setData(newRow, "un", modelViewProdutos.data(row, "un"));
  model.setData(newRow, "un2", modelViewProdutos.data(row, "un2"));
  model.setData(newRow, "prcUnitario", custo);
  model.setData(newRow, "kgcx", modelViewProdutos.data(row, "kgcx"));
  model.setData(newRow, "formComercial", modelViewProdutos.data(row, "formComercial"));
  model.setData(newRow, "codComercial", modelViewProdutos.data(row, "codComercial"));
  model.setData(newRow, "codBarras", modelViewProdutos.data(row, "codBarras"));
  model.setData(newRow, "dataPrevCompra", dataPrevista);
  model.setData(newRow, "quant", quant);
  model.setData(newRow, "preco", quant * custo);
  model.setData(newRow, "caixas", quant / step);

  model.submitAll();
}

void ProdutosPendentes::atualizarVenda(const int rowProduto) {
  if (ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) { dividirVenda(ui->doubleSpinBoxComprar->value(), modelProdutos.data(rowProduto, "quant").toDouble(), rowProduto); }

  modelProdutos.setData(rowProduto, "status", "INICIADO");
}

void ProdutosPendentes::dividirCompra(const double quantSeparar, const double quantCompra, const int rowCompra) {
  // NOTE: *quuebralinha pedido_fornecedor2

  const double quantCaixa = modelCompra.data(rowCompra, "quant").toDouble() / modelCompra.data(rowCompra, "caixas").toDouble();
  const double proporcao = quantSeparar / quantCompra;
  const double preco = modelCompra.data(rowCompra, "preco").toDouble();

  // -------------------------------------------------------------------------

  const int newRow = modelCompra.insertRowAtEnd();

  // copiar colunas
  for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
    if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
    if (column == modelCompra.fieldIndex("created")) { continue; }
    if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelCompra.data(rowCompra, column);

    if (value.isNull()) { continue; }

    modelCompra.setData(newRow, column, value);
  }

  // -------------------------------------------------------------------------
  // alterar linha original

  modelCompra.setData(rowCompra, "quant", quantSeparar);
  modelCompra.setData(rowCompra, "caixas", (quantSeparar / quantCaixa));
  modelCompra.setData(rowCompra, "preco", preco * proporcao);

  // -------------------------------------------------------------------------
  // alterar linha nova

  const double proporcaoNovo = (quantCompra - quantSeparar) / quantCompra;

  modelCompra.setData(newRow, "idRelacionado", modelCompra.data(rowCompra, "idVendaProduto2"));
  modelCompra.setData(newRow, "quant", (quantCompra - quantSeparar));
  modelCompra.setData(newRow, "caixas", (quantCompra - quantSeparar) / quantCaixa);
  modelCompra.setData(newRow, "preco", preco * proporcaoNovo);
}

void ProdutosPendentes::dividirVenda(const double quantSeparar, const double quantVenda, const int rowProduto) {
  // NOTE: *quebralinha venda_produto2

  const double quantCaixa = modelProdutos.data(rowProduto, "quantCaixa").toDouble();
  const double proporcao = quantSeparar / quantVenda;
  const double parcial = modelProdutos.data(rowProduto, "parcial").toDouble();
  const double parcialDesc = modelProdutos.data(rowProduto, "parcialDesc").toDouble();
  const double total = modelProdutos.data(rowProduto, "total").toDouble();

  modelProdutos.setData(rowProduto, "quant", quantSeparar);
  modelProdutos.setData(rowProduto, "caixas", (quantSeparar / quantCaixa));
  modelProdutos.setData(rowProduto, "parcial", parcial * proporcao);
  modelProdutos.setData(rowProduto, "parcialDesc", parcialDesc * proporcao);
  modelProdutos.setData(rowProduto, "total", total * proporcao);

  // -------------------------------------------------------------------------

  const int newRow = modelProdutos.insertRowAtEnd();

  // copiar colunas
  for (int column = 0, columnCount = modelProdutos.columnCount(); column < columnCount; ++column) {
    if (column == modelProdutos.fieldIndex("idVendaProduto2")) { continue; }
    if (column == modelProdutos.fieldIndex("created")) { continue; }
    if (column == modelProdutos.fieldIndex("lastUpdated")) { continue; }

    const QVariant value = modelProdutos.data(rowProduto, column);

    if (value.isNull()) { continue; }

    modelProdutos.setData(newRow, column, value);
  }

  // -------------------------------------------------------------------------

  // alterar quant, precos, etc da linha nova
  const double proporcaoNovo = (quantVenda - quantSeparar) / quantVenda;

  modelProdutos.setData(newRow, "idRelacionado", modelProdutos.data(rowProduto, "idVendaProduto2"));
  modelProdutos.setData(newRow, "quant", (quantVenda - quantSeparar));
  modelProdutos.setData(newRow, "caixas", (quantVenda - quantSeparar) / quantCaixa);
  modelProdutos.setData(newRow, "parcial", parcial * proporcaoNovo);
  modelProdutos.setData(newRow, "parcialDesc", parcialDesc * proporcaoNovo);
  modelProdutos.setData(newRow, "total", total * proporcaoNovo);
}

// TODO: se o estoque for consumido gerar comissao 2% senao gerar comissao padrao
