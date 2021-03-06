#include "widgetcompraconfirmar.h"
#include "ui_widgetcompraconfirmar.h"

#include "application.h"
#include "cancelaproduto.h"
#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDate>
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

  modelViewCompras.setTable("view_compras");

  modelViewCompras.setFilter("");

  modelViewCompras.setSort("OC");

  modelViewCompras.setHeaderData("dataPrevConf", "Prev. Conf.");

  ui->table->setModel(&modelViewCompras);

  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->table->hideColumn("Compra");
}

void WidgetCompraConfirmar::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked, connectionType);
  connect(ui->pushButtonConfirmarCompra, &QPushButton::clicked, this, &WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked, connectionType);
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

  modelResumo.select();

  modelViewCompras.select();
}

void WidgetCompraConfirmar::resetTables() { modelIsSet = false; }

void WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked() {
  // TODO: ao preencher na tabela de compras colocar o grupo como 'produto/venda'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  const int row = list.first().row();

  const QString idCompra = modelViewCompras.data(row, "Compra").toString();
  const QString idVenda = modelViewCompras.data(row, "Venda").toString();

  InputDialogFinanceiro inputDlg(InputDialogFinanceiro::Tipo::ConfirmarCompra, this);
  inputDlg.setFilter(idCompra);

  if (inputDlg.exec() != QDialog::Accepted) { return; }

  const QDate dataPrevista = inputDlg.getDate();
  const QDate dataConf = inputDlg.getNextDate();

  qApp->startTransaction("WidgetCompraConfirmar::on_pushButtonConfirmarCompra");

  confirmarCompra(idCompra, dataPrevista, dataConf);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  updateTables();
}

void WidgetCompraConfirmar::confirmarCompra(const QString &idCompra, const QDate &dataPrevista, const QDate &dataConf) {
  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat WHERE status = 'EM COMPRA' AND idVendaProduto2 IN (SELECT "
                     "idVendaProduto2 FROM pedido_fornecedor_has_produto2 WHERE idCompra = :idCompra AND selecionado = TRUE)");
  queryVenda.bindValue(":dataRealConf", dataConf);
  queryVenda.bindValue(":dataPrevFat", dataPrevista);
  queryVenda.bindValue(":idCompra", idCompra);

  if (not queryVenda.exec()) { throw RuntimeException("Erro salvando status da venda: " + queryVenda.lastError().text()); }

  //------------------------------------------------

  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat, selecionado = FALSE WHERE status = 'EM COMPRA' "
                      "AND idCompra = :idCompra AND selecionado = TRUE");
  queryCompra.bindValue(":dataRealConf", dataConf);
  queryCompra.bindValue(":dataPrevFat", dataPrevista);
  queryCompra.bindValue(":idCompra", idCompra);

  if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando status da compra: " + queryCompra.lastError().text()); }
}

void WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  auto cancelaProduto = new CancelaProduto(CancelaProduto::Tipo::CompraConfirmar, this);
  cancelaProduto->setAttribute(Qt::WA_DeleteOnClose);
  cancelaProduto->setFilter(modelViewCompras.data(list.first().row(), "OC").toString());
}

// TODO: 1poder confirmar dois pedidos juntos (quando vem um espelho só) (cancelar os pedidos e fazer um pedido só?)
// TODO: 1permitir na tela de compras alterar uma venda para quebrar um produto em dois para os casos de lotes diferentes: 50 -> 40+10
// TODO: 0colocar data para frete/st e se elas são inclusas nas parcelas ou separadas
// TODO: 0mesmo bug do gerarcompra/produtospendentes em que o prcUnitario é multiplicado pela quantidade total e nao a da linha
// TODO: 0cancelar nesta tela nao altera status para pendente
