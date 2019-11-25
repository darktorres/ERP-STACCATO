#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "inputdialog.h"
#include "log.h"
#include "produtospendentes.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "ui_produtospendentes.h"

ProdutosPendentes::ProdutosPendentes(const QString &codComercial, const QString &idVenda, QWidget *parent) : QDialog(parent), ui(new Ui::ProdutosPendentes) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  connect(ui->pushButtonComprar, &QPushButton::clicked, this, &ProdutosPendentes::on_pushButtonComprar_clicked);
  connect(ui->pushButtonConsumirEstoque, &QPushButton::clicked, this, &ProdutosPendentes::on_pushButtonConsumirEstoque_clicked);
  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ProdutosPendentes::recalcularQuantidade);

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

  if (not modelProdutos.select()) { return; }

  modelViewProdutos.setFilter("codComercial = '" + codComercial + "' AND idVenda = '" + idVenda + "' AND status IN ('PENDENTE', 'REPO. ENTREGA', 'REPO. RECEB.')");

  if (not modelViewProdutos.select()) { return; }

  ui->doubleSpinBoxQuantTotal->setSuffix(" " + modelViewProdutos.data(0, "un").toString());
  ui->doubleSpinBoxComprar->setSuffix(" " + modelViewProdutos.data(0, "un").toString());

  QSqlQuery query;
  query.prepare("SELECT unCaixa FROM venda_has_produto2 WHERE `idVendaProduto2` = :idVendaProduto2");
  query.bindValue(":idVendaProduto2", modelViewProdutos.data(0, "idVendaProduto2"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando unCaixa: " + query.lastError().text(), this); }

  const double step = query.value("unCaixa").toDouble();

  ui->doubleSpinBoxQuantTotal->setSingleStep(step);
  ui->doubleSpinBoxComprar->setSingleStep(step);

  const QString fornecedor = modelViewProdutos.data(0, "fornecedor").toString();

  modelEstoque.setQuery(
      "SELECT `e`.`status` AS `status`, `e`.`idEstoque` AS `idEstoque`, `e`.`descricao` AS `descricao`, (`e`.`quant` + COALESCE(SUM(`ec`.`quant`), 0)) AS `restante`, `e`.`un` AS "
      "`unEst`, IF(((`p`.`un` = 'M²') OR (`p`.`un` = 'M2') OR (`p`.`un` = 'ML')), ((`e`.`quant` + COALESCE(SUM(`ec`.`quant`), 0)) / `p`.`m2cx`), ((`e`.`quant` + COALESCE(SUM(`ec`.`quant`), 0)) / "
      "`p`.`pccx`)) AS `Caixas`, `e`.`lote` AS `lote`, `e`.`local` AS `local`, `e`.`bloco` AS `bloco`, `e`.`codComercial` AS `codComercial` FROM `estoque` `e` LEFT JOIN `estoque_has_consumo` `ec` ON "
      "`e`.`idEstoque` = `ec`.`idEstoque` LEFT JOIN `produto` `p` ON `e`.`idProduto` = `p`.`idProduto` LEFT JOIN `venda_has_produto2` `vp2` ON `ec`.`idVendaProduto2` = `vp2`.`idVendaProduto2` WHERE "
      "e.status NOT IN ('CANCELADO', 'QUEBRADO') AND p.fornecedor = '" +
      fornecedor + "' AND e.codComercial = '" + codComercial + "' GROUP BY `e`.`idEstoque` HAVING restante > 0");

  if (modelEstoque.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela estoque: " + modelEstoque.lastError().text(), this); }

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
  ui->tableEstoque->setItemDelegateForColumn("restante", new DoubleDelegate(this, 3));

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
  modelViewProdutos.setHeaderData("idVenda", "Venda");
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

  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(this, 3));
  ui->tableProdutos->setItemDelegateForColumn("custo", new ReaisDelegate(this));

  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("idVendaProdutoFK");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("idCompra");
}

bool ProdutosPendentes::comprar(const QModelIndexList &list, const QDate &dataPrevista) {
  for (const auto &index : list) {
    const int row = index.row();

    if (not atualizarVenda(row)) { return false; }
    if (not enviarProdutoParaCompra(row, dataPrevista)) { return false; }
    if (not enviarExcedenteParaCompra(row, dataPrevista)) { return false; }
  }

  return modelProdutos.submitAll();
}

void ProdutosPendentes::recarregarTabelas() {
  if (not modelProdutos.select()) { return; }
  if (not modelViewProdutos.select()) { return; }

  modelEstoque.setQuery(modelEstoque.query().executedQuery());

  if (modelEstoque.lastError().isValid()) { return qApp->enqueueError("Erro recarregando modelEstoque: " + modelEstoque.lastError().text(), this); }

  ui->tableProdutos->clearSelection();

  if (modelViewProdutos.rowCount() == 1) { ui->tableProdutos->selectRow(0); }
  if (modelViewProdutos.rowCount() == 0) { close(); }
}

void ProdutosPendentes::on_pushButtonComprar_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum produto selecionado!", this); }

  if (list.size() > 1 and ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) { return qApp->enqueueError("Para comprar menos que o total selecione uma linha por vez!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxComprar->value())) { return qApp->enqueueError("Quantidade 0!", this); }

  const QString idVenda = modelViewProdutos.data(list.first().row(), "idVenda").toString();

  InputDialog inputDlg(InputDialog::Tipo::Carrinho, this);
  if (inputDlg.exec() != InputDialog::Accepted) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not comprar(list, inputDlg.getNextDate())) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Produto enviado para carrinho!", this);

  recarregarTabelas();
}

bool ProdutosPendentes::consumirEstoque(const int rowProduto, const int rowEstoque, const double quantConsumir, const double quantVenda) {
  // TODO: 1pensar em alguma forma de poder consumir compra que nao foi faturada ainda

  auto *estoque = new Estoque(modelEstoque.data(rowEstoque, "idEstoque").toString(), false, this);
  if (not estoque->criarConsumo(modelViewProdutos.data(rowProduto, "idVendaProduto2").toInt(), quantConsumir)) { return false; }

  if (quantConsumir < quantVenda) {
    if (not dividirVenda(quantConsumir, quantVenda, rowProduto)) { return false; }
  }

  if (not modelProdutos.setData(rowProduto, "status", modelEstoque.data(rowEstoque, "status"))) { return false; }

  if (not modelProdutos.submitAll()) { return false; }

  return true;
}

void ProdutosPendentes::on_pushButtonConsumirEstoque_clicked() {
  const auto listProduto = ui->tableProdutos->selectionModel()->selectedRows();

  if (listProduto.size() > 1) { return qApp->enqueueError("Selecione apenas um item na tabela de produtos!", this); }

  if (listProduto.isEmpty()) { return qApp->enqueueError("Nenhum produto selecionado!", this); }

  const auto listEstoque = ui->tableEstoque->selectionModel()->selectedRows();

  if (listEstoque.isEmpty()) { return qApp->enqueueError("Nenhum estoque selecionado!", this); }

  const int rowProduto = listProduto.first().row();
  const int rowEstoque = listEstoque.first().row();

  const double quantVenda = modelViewProdutos.data(rowProduto, "quant").toDouble();
  const double quantEstoque = modelEstoque.data(rowEstoque, "restante").toDouble();

  bool ok;

  const double quantConsumir =
      QInputDialog::getDouble(this, "Consumo", "Quantidade a consumir: ", quantVenda, 0, qMin(quantVenda, quantEstoque), 3, &ok, Qt::WindowFlags(), ui->doubleSpinBoxComprar->singleStep());
  if (not ok) { return; }

  const QString idVenda = modelViewProdutos.data(rowProduto, "idVenda").toString();

  if (not qApp->startTransaction()) { return; }

  if (not consumirEstoque(rowProduto, rowEstoque, quantConsumir, quantVenda)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Consumo criado com sucesso!", this);

  recarregarTabelas();
}

bool ProdutosPendentes::enviarExcedenteParaCompra(const int row, const QDate &dataPrevista) {
  const double excedente = ui->doubleSpinBoxComprar->value() - ui->doubleSpinBoxQuantTotal->value();

  if (excedente > 0.) {
    QSqlQuery query;
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
    query.bindValue(":caixas", excedente / modelProdutos.data(row, "unCaixa").toDouble());
    query.bindValue(":prcUnitario", modelViewProdutos.data(row, "custo").toDouble());
    query.bindValue(":preco", modelViewProdutos.data(row, "custo").toDouble() * excedente);
    query.bindValue(":kgcx", modelViewProdutos.data(row, "kgcx"));
    query.bindValue(":formComercial", modelViewProdutos.data(row, "formComercial"));
    query.bindValue(":codComercial", modelViewProdutos.data(row, "codComercial"));
    query.bindValue(":codBarras", modelViewProdutos.data(row, "codBarras"));
    query.bindValue(":dataPrevCompra", dataPrevista);

    if (not query.exec()) { return qApp->enqueueError(false, "Erro inserindo dados em pedido_fornecedor_has_produto: " + query.lastError().text(), this); }
  }

  return true;
}

bool ProdutosPendentes::enviarProdutoParaCompra(const int row, const QDate &dataPrevista) {
  QSqlQuery query;
  query.prepare("INSERT INTO pedido_fornecedor_has_produto (idVenda, idVendaProduto1, fornecedor, idProduto, descricao, obs, colecao, quant, un, un2, caixas, prcUnitario, preco, kgcx, formComercial, "
                "codComercial, codBarras, dataPrevCompra) VALUES (:idVenda, :idVendaProduto1, :fornecedor, :idProduto, :descricao, :obs, :colecao, :quant, :un, :un2, :caixas, :prcUnitario, :preco, "
                ":kgcx, :formComercial, :codComercial, :codBarras, :dataPrevCompra)");
  query.bindValue(":idVenda", modelViewProdutos.data(row, "idVenda"));
  query.bindValue(":idVendaProduto1", modelViewProdutos.data(row, "idVendaProdutoFK"));
  query.bindValue(":fornecedor", modelViewProdutos.data(row, "fornecedor"));
  query.bindValue(":idProduto", modelViewProdutos.data(row, "idProduto"));
  query.bindValue(":descricao", modelViewProdutos.data(row, "produto"));
  query.bindValue(":obs", modelViewProdutos.data(row, "obs"));
  query.bindValue(":colecao", modelViewProdutos.data(row, "colecao"));
  query.bindValue(":un", modelViewProdutos.data(row, "un"));
  query.bindValue(":un2", modelViewProdutos.data(row, "un2"));
  query.bindValue(":prcUnitario", modelViewProdutos.data(row, "custo").toDouble());
  query.bindValue(":kgcx", modelViewProdutos.data(row, "kgcx"));
  query.bindValue(":formComercial", modelViewProdutos.data(row, "formComercial"));
  query.bindValue(":codComercial", modelViewProdutos.data(row, "codComercial"));
  query.bindValue(":codBarras", modelViewProdutos.data(row, "codBarras"));
  query.bindValue(":dataPrevCompra", dataPrevista);

  if (ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) { // compra avulsa para menos
    const QDecDouble quant = ui->doubleSpinBoxComprar->value();
    const QDecDouble custo = modelViewProdutos.data(row, "custo").toDouble();
    const QDecDouble step = ui->doubleSpinBoxQuantTotal->singleStep();

    query.bindValue(":quant", quant.toDouble());
    query.bindValue(":preco", (quant * custo).toDouble());
    query.bindValue(":caixas", (quant / step).toDouble());
  } else {
    const QDecDouble quant = modelViewProdutos.data(row, "quant").toDouble();
    const QDecDouble custo = modelViewProdutos.data(row, "custo").toDouble();
    const QDecDouble step = ui->doubleSpinBoxQuantTotal->singleStep();

    query.bindValue(":quant", quant.toDouble());
    query.bindValue(":preco", (quant * custo).toDouble());
    query.bindValue(":caixas", (quant / step).toDouble());
  }

  if (not query.exec()) { return qApp->enqueueError(false, "Erro inserindo dados em pedido_fornecedor_has_produto: " + query.lastError().text(), this); }

  return true;
}

bool ProdutosPendentes::atualizarVenda(const int rowProduto) {
  if (ui->doubleSpinBoxComprar->value() < ui->doubleSpinBoxQuantTotal->value()) {
    if (not dividirVenda(ui->doubleSpinBoxComprar->value(), modelProdutos.data(rowProduto, "quant").toDouble(), rowProduto)) { return false; }
  }

  if (not modelProdutos.setData(rowProduto, "status", "INICIADO")) { return false; }

  return true;
}

bool ProdutosPendentes::dividirVenda(const QDecDouble quantSeparar, const QDecDouble quantVenda, const int rowProduto) {
  const int newRow = modelProdutos.insertRowAtEnd();

  // copiar colunas
  for (int column = 0, columnCount = modelProdutos.columnCount(); column < columnCount; ++column) {
    if (modelProdutos.fieldIndex("idVendaProduto2") == column) { continue; }
    if (modelProdutos.fieldIndex("created") == column) { continue; }
    if (modelProdutos.fieldIndex("lastUpdated") == column) { continue; }

    const QVariant value = modelProdutos.data(rowProduto, column);

    if (not modelProdutos.setData(newRow, column, value)) { return false; }
  }

  const QDecDouble unCaixa = modelProdutos.data(rowProduto, "unCaixa").toDouble();

  const QDecDouble proporcao = quantSeparar / quantVenda;
  const QDecDouble parcial = QDecDouble(modelProdutos.data(rowProduto, "parcial").toDouble()) * proporcao;
  const QDecDouble parcialDesc = QDecDouble(modelProdutos.data(rowProduto, "parcialDesc").toDouble()) * proporcao;
  const QDecDouble total = QDecDouble(modelProdutos.data(rowProduto, "total").toDouble()) * proporcao;

  if (not modelProdutos.setData(rowProduto, "quant", quantSeparar.toDouble())) { return false; }
  if (not modelProdutos.setData(rowProduto, "caixas", (quantSeparar / unCaixa).toDouble())) { return false; }
  if (not modelProdutos.setData(rowProduto, "parcial", parcial.toDouble())) { return false; }
  if (not modelProdutos.setData(rowProduto, "parcialDesc", parcialDesc.toDouble())) { return false; }
  if (not modelProdutos.setData(rowProduto, "total", total.toDouble())) { return false; }

  // alterar quant, precos, etc da linha nova
  const QDecDouble proporcaoNovo = (quantVenda - quantSeparar) / quantVenda;
  const QDecDouble parcialNovo = QDecDouble(modelProdutos.data(newRow, "parcial").toDouble()) * proporcaoNovo;
  const QDecDouble parcialDescNovo = QDecDouble(modelProdutos.data(newRow, "parcialDesc").toDouble()) * proporcaoNovo;
  const QDecDouble totalNovo = QDecDouble(modelProdutos.data(newRow, "total").toDouble()) * proporcaoNovo;

  if (not modelProdutos.setData(newRow, "idRelacionado", modelProdutos.data(rowProduto, "idVendaProduto2"))) { return false; }
  if (not modelProdutos.setData(newRow, "quant", (quantVenda - quantSeparar).toDouble())) { return false; }
  if (not modelProdutos.setData(newRow, "caixas", ((quantVenda - quantSeparar) / unCaixa).toDouble())) { return false; }
  if (not modelProdutos.setData(newRow, "parcial", parcialNovo.toDouble())) { return false; }
  if (not modelProdutos.setData(newRow, "parcialDesc", parcialDescNovo.toDouble())) { return false; }
  if (not modelProdutos.setData(newRow, "total", totalNovo.toDouble())) { return false; }

  return true;
}

// NOTE: se o estoque for consumido gerar comissao 2% senao gerar comissao padrao
