#include "widgetcompraconfirmar.h"
#include "ui_widgetcompraconfirmar.h"

#include "application.h"
#include "cancelaproduto.h"
#include "financeiroproxymodel.h"
#include "followup.h"
#include "inputdialogproduto.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

WidgetCompraConfirmar::WidgetCompraConfirmar(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraConfirmar) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetCompraConfirmar::~WidgetCompraConfirmar() { delete ui; }

void WidgetCompraConfirmar::setupTables() {
  modelResumo.setTable("view_fornecedor_compra_confirmar");

  modelResumo.setFilter("");

  ui->tableResumo->setModel(&modelResumo);

  //---------------------------------------------------------------------------------------

  modelCompras.setTable("view_compras");

  modelCompras.setHeaderData("OC", "O.C.");

  modelCompras.setFilter("");

  modelCompras.setHeaderData("prazoEntrega", "Prazo Limite");
  modelCompras.setHeaderData("novoPrazoEntrega", "Novo Prazo");
  modelCompras.setHeaderData("dataPrevConf", "Prev. Conf.");

  modelCompras.proxyModel = new FinanceiroProxyModel(&modelCompras, this);

  ui->table->setModel(&modelCompras);

  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->table->hideColumn("Compra");
}

void WidgetCompraConfirmar::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked, connectionType);
  connect(ui->pushButtonConfirmarCompra, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonLimparFiltro, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonLimparFiltro_clicked, connectionType);
  connect(ui->table, &TableView::doubleClicked, this, &WidgetCompraConfirmar::on_table_doubleClicked, connectionType);
  connect(ui->tableResumo, &TableView::clicked, this, &WidgetCompraConfirmar::on_tableResumo_clicked, connectionType);
}

void WidgetCompraConfirmar::updateTables() {
  if (not isSet) {
    setupTables();
    setConnections();
    isSet = true;
  }

  modelResumo.select();

  modelCompras.select();
}

void WidgetCompraConfirmar::resetTables() { setupTables(); }

void WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked() {
  // TODO: ao preencher na tabela de compras colocar o grupo como 'produto/venda'

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }
  if (selection.size() > 1) { throw RuntimeError("Deve selecionar apenas uma linha!", this); }

  const int row = selection.first().row();

  const QString ordemCompra = modelCompras.data(row, "OC").toString();
  const QString idVenda = modelCompras.data(row, "Venda").toString();

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::ConfirmarCompra, this);
  inputDlg.setFilter({ordemCompra});

  if (inputDlg.exec() != QDialog::Accepted) { return; }

  const QDate dataPrevista = inputDlg.getDate();
  const QDate dataConf = inputDlg.getNextDate();

  qApp->startTransaction("WidgetCompraConfirmar::on_pushButtonConfirmarCompra");

  confirmarCompra(ordemCompra, dataPrevista, dataConf);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  updateTables();
}

void WidgetCompraConfirmar::confirmarCompra(const QString &ordemCompra, const QDate dataPrevista, const QDate dataConf) {
  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat WHERE status = 'EM COMPRA' AND idVendaProduto2 IN (SELECT "
                     "idVendaProduto2 FROM pedido_fornecedor_has_produto2 WHERE ordemCompra = :ordemCompra AND selecionado = TRUE)");
  queryVenda.bindValue(":dataRealConf", dataConf);
  queryVenda.bindValue(":dataPrevFat", dataPrevista);
  queryVenda.bindValue(":ordemCompra", ordemCompra);

  if (not queryVenda.exec()) { throw RuntimeException("Erro salvando status da venda: " + queryVenda.lastError().text()); }

  //------------------------------------------------

  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat, selecionado = FALSE WHERE status = 'EM COMPRA' "
                      "AND ordemCompra = :ordemCompra AND selecionado = TRUE");
  queryCompra.bindValue(":dataRealConf", dataConf);
  queryCompra.bindValue(":dataPrevFat", dataPrevista);
  queryCompra.bindValue(":ordemCompra", ordemCompra);

  if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando status da compra: " + queryCompra.lastError().text()); }
}

void WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  auto *cancelaProduto = new CancelaProduto(CancelaProduto::Tipo::CompraConfirmar, this);
  cancelaProduto->setAttribute(Qt::WA_DeleteOnClose);
  cancelaProduto->setFilter(modelCompras.data(selection.first().row(), "OC").toString());
}

void WidgetCompraConfirmar::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString ordemCompra = modelCompras.data(selection.first().row(), "OC").toString();

  auto *followup = new FollowUp(ordemCompra, FollowUp::Tipo::Compra, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetCompraConfirmar::on_tableResumo_clicked(const QModelIndex &index) {
  const QString fornecedor = index.isValid() ? modelResumo.data(index.row(), "fornecedor").toString() : "";

  const QString filtro = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";

  modelCompras.setFilter(filtro);
}

void WidgetCompraConfirmar::on_pushButtonLimparFiltro_clicked() {
  ui->tableResumo->clearSelection();

  const QString fornecedor = "";

  const QString filtro = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";

  modelCompras.setFilter(filtro);
}

void WidgetCompraConfirmar::on_table_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelCompras.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelCompras.data(index.row(), "Venda")); }
}

// TODO: 1poder confirmar dois pedidos juntos (quando vem um espelho só) (cancelar os pedidos e fazer um pedido só?)
// TODO: 1permitir na tela de compras alterar uma venda para quebrar um produto em dois para os casos de lotes diferentes: 50 -> 40+10
// TODO: 0colocar data para frete/st e se elas são inclusas nas parcelas ou separadas
// TODO: 0mesmo bug do gerarcompra/produtospendentes em que o prcUnitario é multiplicado pela quantidade total e nao a da linha
// TODO: 0cancelar nesta tela nao altera status para pendente
