#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "ui_widgetcompraconfirmar.h"
#include "widgetcompraconfirmar.h"

WidgetCompraConfirmar::WidgetCompraConfirmar(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraConfirmar) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetCompraConfirmar::~WidgetCompraConfirmar() { delete ui; }

void WidgetCompraConfirmar::setupTables() {
  modelResumo.setTable("view_fornecedor_compra_confirmar");

  ui->tableResumo->setModel(&modelResumo);

  //---------------------------------------------------------------------------------------

  modelViewCompras.setTable("view_compras");

  modelViewCompras.setHeaderData("dataPrevConf", "Prev. Conf.");

  ui->table->setModel(&modelViewCompras);
  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));
  ui->table->hideColumn("Compra");
}

void WidgetCompraConfirmar::setConnections() {
  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked);
  connect(ui->pushButtonConfirmarCompra, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked);
  connect(ui->table, &TableView::entered, this, &WidgetCompraConfirmar::on_table_entered);
}

void WidgetCompraConfirmar::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (not modelResumo.select()) { return; }

  ui->tableResumo->resizeColumnsToContents();

  if (not modelViewCompras.select()) { return; }

  ui->table->resizeColumnsToContents();
}

void WidgetCompraConfirmar::resetTables() { modelIsSet = false; }

void WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked() {
  // TODO: ao preencher na tabela de compras colocar o grupo como 'produto/venda'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  const int row = list.first().row();

  const QString idCompra = modelViewCompras.data(row, "Compra").toString();
  const QString idVenda = modelViewCompras.data(row, "Venda").toString();

  InputDialogFinanceiro inputDlg(InputDialogFinanceiro::Tipo::ConfirmarCompra);
  if (not inputDlg.setFilter(idCompra)) { return; }

  if (inputDlg.exec() != InputDialogFinanceiro::Accepted) { return; }

  const QDateTime dataPrevista = inputDlg.getDate();
  const QDateTime dataConf = inputDlg.getNextDate();

  if (not qApp->startTransaction()) { return; }

  if (not confirmarCompra(idCompra, dataPrevista, dataConf)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Compra confirmada!");
}

bool WidgetCompraConfirmar::confirmarCompra(const QString &idCompra, const QDateTime &dataPrevista, const QDateTime &dataConf) {
  // REFAC: prepare querys beforehand

  QSqlQuery query1;
  query1.prepare("SELECT idPedido, idVendaProduto FROM pedido_fornecedor_has_produto WHERE idCompra = :idCompra AND selecionado = TRUE");
  query1.bindValue(":idCompra", idCompra);

  if (not query1.exec()) { return qApp->enqueueError(false, "Erro buscando produtos: " + query1.lastError().text()); }

  QSqlQuery queryUpdate1;
  queryUpdate1.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat, selecionado = FALSE WHERE idPedido = :idPedido");

  QSqlQuery queryUpdate2;
  queryUpdate2.prepare("UPDATE venda_has_produto SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat WHERE idVendaProduto = :idVendaProduto "
                       "AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  while (query1.next()) {
    queryUpdate1.bindValue(":dataRealConf", dataConf);
    queryUpdate1.bindValue(":dataPrevFat", dataPrevista);
    queryUpdate1.bindValue(":idPedido", query1.value("idPedido"));

    if (not queryUpdate1.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + queryUpdate1.lastError().text()); }

    if (query1.value("idVendaProduto").toInt() != 0) {
      queryUpdate2.bindValue(":dataRealConf", dataConf);
      queryUpdate2.bindValue(":dataPrevFat", dataPrevista);
      queryUpdate2.bindValue(":idVendaProduto", query1.value("idVendaProduto"));

      if (not queryUpdate2.exec()) { return qApp->enqueueError(false, "Erro salvando status da venda: " + queryUpdate2.lastError().text()); }
    }
  }

  return true;
}

void WidgetCompraConfirmar::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

bool WidgetCompraConfirmar::cancelar(const int row) {
  QSqlQuery query1;
  query1.prepare("SELECT idVendaProduto FROM pedido_fornecedor_has_produto WHERE ordemCompra = :ordemCompra AND status = 'EM COMPRA'");
  query1.bindValue(":ordemCompra", modelViewCompras.data(row, "OC"));

  if (not query1.exec()) { return qApp->enqueueError(false, "Erro buscando dados: " + query1.lastError().text()); }

  QSqlQuery query2;
  query2.prepare(
      "UPDATE venda_has_produto SET status = 'PENDENTE', idCompra = NULL WHERE idVendaProduto = :idVendaProduto AND status = 'EM COMPRA' AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  while (query1.next()) {
    query2.bindValue(":idVendaProduto", query1.value("idVendaProduto"));

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro voltando status do produto: " + query2.lastError().text()); }
  }

  QSqlQuery query3;
  query3.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'CANCELADO' WHERE ordemCompra = :ordemCompra AND status = 'EM COMPRA'");
  query3.bindValue(":ordemCompra", modelViewCompras.data(row, "OC"));

  if (not query3.exec()) { return qApp->enqueueError(false, "Erro salvando dados: " + query3.lastError().text()); }

  return true;
}

void WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked() {
  // TODO: 5cancelar itens individuais no lugar da compra toda?

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  const int row = list.first().row();

  const QString idVenda = modelViewCompras.data(row, "idVenda").toString();

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Essa operação ira cancelar todos os itens desta OC, mesmo os já confirmados/faturados! Deseja continuar?", QMessageBox::Yes | QMessageBox::No,
                     this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not cancelar(row)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Itens cancelados!");
}

// TODO: 1poder confirmar dois pedidos juntos (quando vem um espelho só) (cancelar os pedidos e fazer um pedido só?)
// TODO: 1permitir na tela de compras alterar uma venda para quebrar um produto em dois para os casos de lotes diferentes: 50 -> 40+10
// TODO: 0colocar data para frete/st e se elas são inclusas nas parcelas ou separadas
// TODO: 0mesmo bug do gerarcompra/produtospendentes em que o prcUnitario é multiplicado pela quantidade total e nao a da linha
// TODO: 0cancelar nesta tela nao altera status para pendente
