#include "produtospendentes.h"
#include "ui_produtospendentes.h"

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "inputdialog.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

ProdutosPendentes::ProdutosPendentes(const QString &codComercial, const QString &idVenda, QWidget *parent) : QDialog(parent), ui(new Ui::ProdutosPendentes) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  connect(ui->pushButtonComprar, &QPushButton::clicked, this, &ProdutosPendentes::on_pushButtonComprar_clicked);
  connect(ui->pushButtonConsumirEstoque, &QPushButton::clicked, this, &ProdutosPendentes::on_pushButtonConsumirEstoque_clicked);
  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ProdutosPendentes::recalcularQuantidade);

  setWindowTitle("Venda " + idVenda);

  viewProduto(codComercial, idVenda);

  show();
}

ProdutosPendentes::~ProdutosPendentes() { delete ui; }

void ProdutosPendentes::recalcularQuantidade() {
  const auto selectedRows = ui->tableProdutos->selectionModel()->selectedRows();
  double quant = 0;

  for (const auto &index : selectedRows) { quant += modelViewProdutos.data(index.row(), "quant").toDouble(); }

  ui->doubleSpinBoxQuantTotal->setValue(quant);
  ui->doubleSpinBoxComprar->setValue(quant);
}

void ProdutosPendentes::viewProduto(const QString &codComercial, const QString &idVenda) {
  modelProdutos.setFilter("codComercial = '" + codComercial + "' AND idVenda = '" + idVenda + "' AND status IN ('PENDENTE', 'REPO. ENTREGA', 'REPO. RECEB.')");

  modelProdutos.select();

  modelViewProdutos.setFilter("codComercial = '" + codComercial + "' AND idVenda = '" + idVenda + "' AND status IN ('PENDENTE', 'REPO. ENTREGA', 'REPO. RECEB.')");

  modelViewProdutos.select();

  ui->doubleSpinBoxQuantTotal->setSuffix(" " + modelViewProdutos.data(0, "un").toString());
  ui->doubleSpinBoxComprar->setSuffix(" " + modelViewProdutos.data(0, "un").toString());

  SqlQuery query;
  query.prepare("SELECT quantCaixa FROM venda_has_produto2 WHERE `idVendaProduto2` = :idVendaProduto2");
  query.bindValue(":idVendaProduto2", modelViewProdutos.data(0, "idVendaProduto2"));

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando quantCaixa: " + query.lastError().text(), this); }

  const double step = query.value("quantCaixa").toDouble();

  ui->doubleSpinBoxQuantTotal->setSingleStep(step);
  ui->doubleSpinBoxComprar->setSingleStep(step);

  const QString fornecedor = modelViewProdutos.data(0, "fornecedor").toString();

  modelEstoque.setQuery(
      "SELECT `e`.`status` AS `status`, `e`.`idEstoque` AS `idEstoque`, `e`.`descricao` AS `descricao`, e.restante, `e`.`un` AS `unEst`, e.restante / p.quantCaixa AS `Caixas`, `e`.`lote` AS `lote`, "
      "`e`.`local` AS `local`, `e`.`bloco` AS `bloco`, `e`.`codComercial` AS `codComercial` FROM `estoque` `e` LEFT JOIN `produto` `p` ON `e`.`idProduto` = `p`.`idProduto` WHERE e.status NOT IN "
      "('CANCELADO' , 'QUEBRADO') AND p.fornecedor = '" +
      fornecedor + "' AND e.codComercial = '" + codComercial + "' GROUP BY `e`.`idEstoque` HAVING restante > 0");

  if (modelEstoque.lastError().isValid()) { throw RuntimeException("Erro lendo tabela estoque: " + modelEstoque.lastError().text(), this); }

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
  modelViewProdutos.setHeaderData("obs", "Obs.");

  ui->tableProdutos->setModel(&modelViewProdutos);

  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(3, this));
  ui->tableProdutos->setItemDelegateForColumn("custo", new ReaisDelegate(this));

  ui->tableProdutos->hideColumn("idVenda");
  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("idVendaProdutoFK");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idCompra");
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

  modelEstoque.setQuery(modelEstoque.query().executedQuery());

  if (modelEstoque.lastError().isValid()) { throw RuntimeException("Erro recarregando modelEstoque: " + modelEstoque.lastError().text(), this); }

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

void ProdutosPendentes::consumirEstoque(const int rowProduto, const int rowEstoque, const double quantConsumir, const double quantVenda) {
  // TODO: 1pensar em alguma forma de poder consumir compra que nao foi faturada ainda

  auto *estoque = new Estoque(modelEstoque.data(rowEstoque, "idEstoque").toString(), false, this);
  estoque->criarConsumo(modelViewProdutos.data(rowProduto, "idVendaProduto2").toInt(), quantConsumir);

  if (quantConsumir < quantVenda) { dividirVenda(quantConsumir, quantVenda, rowProduto); }

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
  model.setData(newRow, "prcUnitario", modelViewProdutos.data(row, "custo"));
  model.setData(newRow, "kgcx", modelViewProdutos.data(row, "kgcx"));
  model.setData(newRow, "formComercial", modelViewProdutos.data(row, "formComercial"));
  model.setData(newRow, "codComercial", modelViewProdutos.data(row, "codComercial"));
  model.setData(newRow, "codBarras", modelViewProdutos.data(row, "codBarras"));
  model.setData(newRow, "dataPrevCompra", dataPrevista);

  const double quant = (ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) ? ui->doubleSpinBoxComprar->value() : modelViewProdutos.data(row, "quant").toDouble();
  const double custo = modelViewProdutos.data(row, "custo").toDouble();
  const double step = ui->doubleSpinBoxQuantTotal->singleStep();

  model.setData(newRow, "quant", quant);
  model.setData(newRow, "preco", quant * custo);
  model.setData(newRow, "caixas", quant / step);

  model.submitAll();
}

void ProdutosPendentes::atualizarVenda(const int rowProduto) {
  if (ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) { dividirVenda(ui->doubleSpinBoxComprar->value(), modelProdutos.data(rowProduto, "quant").toDouble(), rowProduto); }

  modelProdutos.setData(rowProduto, "status", "INICIADO");
}

void ProdutosPendentes::dividirVenda(const double quantSeparar, const double quantVenda, const int rowProduto) {
  // NOTE: *quebralinha venda_has_produto2

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

// NOTE: se o estoque for consumido gerar comissao 2% senao gerar comissao padrao
