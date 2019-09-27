#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "cancelaproduto.h"
#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
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

  modelResumo.setFilter("");

  ui->tableResumo->setModel(&modelResumo);

  //---------------------------------------------------------------------------------------

  modelViewCompras.setTable("view_compras");

  modelViewCompras.setFilter("");

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

  if (not modelResumo.select()) { return; }

  if (not modelViewCompras.select()) { return; }
}

void WidgetCompraConfirmar::resetTables() { modelIsSet = false; }

void WidgetCompraConfirmar::on_pushButtonConfirmarCompra_clicked() {
  // TODO: ao preencher na tabela de compras colocar o grupo como 'produto/venda'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  const int row = list.first().row();

  const QString idCompra = modelViewCompras.data(row, "Compra").toString();

  InputDialogFinanceiro inputDlg(InputDialogFinanceiro::Tipo::ConfirmarCompra, this);
  if (not inputDlg.setFilter(idCompra)) { return; }

  if (inputDlg.exec() != InputDialogFinanceiro::Accepted) { return; }

  const QDateTime dataPrevista = inputDlg.getDate();
  const QDateTime dataConf = inputDlg.getNextDate();

  if (not qApp->startTransaction()) { return; }

  if (not confirmarCompra(idCompra, dataPrevista, dataConf)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Compra confirmada!", this);
}

bool WidgetCompraConfirmar::confirmarCompra(const QString &idCompra, const QDateTime &dataPrevista, const QDateTime &dataConf) {
  QSqlQuery querySelect;
  querySelect.prepare("SELECT idPedido, idVendaProduto FROM pedido_fornecedor_has_produto WHERE idCompra = :idCompra AND selecionado = TRUE");
  querySelect.bindValue(":idCompra", idCompra);

  if (not querySelect.exec()) { return qApp->enqueueError(false, "Erro buscando produtos: " + querySelect.lastError().text(), this); }

  QSqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat, selecionado = FALSE WHERE status = 'EM COMPRA' "
                      "AND idPedido = :idPedido");

  QSqlQuery queryVenda;
  queryVenda.prepare(
      "UPDATE venda_has_produto SET status = 'EM FATURAMENTO', dataRealConf = :dataRealConf, dataPrevFat = :dataPrevFat WHERE status = 'EM COMPRA' AND idVendaProduto = :idVendaProduto");

  while (querySelect.next()) {
    queryCompra.bindValue(":dataRealConf", dataConf);
    queryCompra.bindValue(":dataPrevFat", dataPrevista);
    queryCompra.bindValue(":idPedido", querySelect.value("idPedido"));

    if (not queryCompra.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + queryCompra.lastError().text(), this); }

    if (querySelect.value("idVendaProduto").toInt() != 0) {
      queryVenda.bindValue(":dataRealConf", dataConf);
      queryVenda.bindValue(":dataPrevFat", dataPrevista);
      queryVenda.bindValue(":idVendaProduto", querySelect.value("idVendaProduto"));

      if (not queryVenda.exec()) { return qApp->enqueueError(false, "Erro salvando status da venda: " + queryVenda.lastError().text(), this); }
    }
  }

  return true;
}

void WidgetCompraConfirmar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  auto cancelaProduto = new CancelaProduto(CancelaProduto::Tipo::CompraConfirmar, this);
  cancelaProduto->setAttribute(Qt::WA_DeleteOnClose);
  cancelaProduto->setFilter(modelViewCompras.data(list.first().row(), "OC").toString());
}

// TODO: 1poder confirmar dois pedidos juntos (quando vem um espelho só) (cancelar os pedidos e fazer um pedido só?)
// TODO: 1permitir na tela de compras alterar uma venda para quebrar um produto em dois para os casos de lotes diferentes: 50 -> 40+10
// TODO: 0colocar data para frete/st e se elas são inclusas nas parcelas ou separadas
// TODO: 0mesmo bug do gerarcompra/produtospendentes em que o prcUnitario é multiplicado pela quantidade total e nao a da linha
// TODO: 0cancelar nesta tela nao altera status para pendente
