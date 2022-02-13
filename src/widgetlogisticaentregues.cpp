#include "widgetlogisticaentregues.h"
#include "ui_widgetlogisticaentregues.h"

#include "application.h"
#include "doubledelegate.h"
#include "sortfilterproxymodel.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

WidgetLogisticaEntregues::WidgetLogisticaEntregues(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaEntregues) { ui->setupUi(this); }

WidgetLogisticaEntregues::~WidgetLogisticaEntregues() { delete ui; }

void WidgetLogisticaEntregues::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaEntregues::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->radioButtonEntregaLimpar, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->radioButtonParcialEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->radioButtonSemEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->radioButtonTotalEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->tableProdutos, &QTableView::doubleClicked, this, &WidgetLogisticaEntregues::on_tableProdutos_doubleClicked, connectionType);
  connect(ui->tableVendas, &QTableView::doubleClicked, this, &WidgetLogisticaEntregues::on_tableVendas_doubleClicked, connectionType);
  connect(ui->tableVendas->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaEntregues::on_tableVendas_selectionChanged, connectionType);
}

void WidgetLogisticaEntregues::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelVendas.select();

  // -----------------------------------------------------------------

  modelProdutos.select();
}

void WidgetLogisticaEntregues::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetLogisticaEntregues::montaFiltro() {
  QStringList filtros;

  QString filtroCheck;

  if (ui->radioButtonEntregaLimpar->isChecked()) { filtroCheck = ""; }
  if (ui->radioButtonTotalEntrega->isChecked()) { filtroCheck = "Entregue > 0 AND Estoque = 0 AND Outros = 0"; }
  if (ui->radioButtonParcialEntrega->isChecked()) { filtroCheck = "Entregue > 0 AND (Estoque > 0 OR Outros > 0)"; }
  if (ui->radioButtonSemEntrega->isChecked()) { filtroCheck = "Entregue = 0"; }

  if (not filtroCheck.isEmpty()) { filtros << filtroCheck; }

  //-------------------------------------

  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "idVenda LIKE '%" + textoBusca + "%'";
  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelVendas.setFilter(filtros.join(" AND "));
}

void WidgetLogisticaEntregues::setupTables() {
  modelVendas.setTable("view_entrega");

  modelVendas.setHeaderData("idVenda", "Venda");
  modelVendas.setHeaderData("prazoEntrega", "Prazo Limite");

  ui->tableVendas->setModel(&modelVendas);

  ui->tableVendas->setItemDelegate(new DoubleDelegate(this));
}

void WidgetLogisticaEntregues::on_tableVendas_selectionChanged() {
  const auto selection = ui->tableVendas->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  // TODO: move query to Sql class
  modelProdutos.setQuery(
      "SELECT `vp2`.`idVendaProduto2` AS `idVendaProduto2`, `vp2`.`idProduto` AS `idProduto`, `vp2`.`dataPrevEnt` AS `dataPrevEnt`, `vp2`.`dataRealEnt` AS `dataRealEnt`, `vp2`.`status` "
      "AS `status`, `vp2`.`fornecedor` AS `fornecedor`, `vp2`.`idVenda` AS `idVenda`, `vp2`.`produto` AS `produto`, `vp2`.`caixas` AS `caixas`, `vp2`.`quant` AS `quant`, `vp2`.`un` AS "
      "`un`, `vp2`.`quantCaixa` AS `quantCaixa`, `vp2`.`codComercial` AS `codComercial`, `vp2`.`formComercial` AS `formComercial`, GROUP_CONCAT(DISTINCT`ehc`.`idConsumo`) AS `idConsumo`, "
      "vp2.obs FROM (`venda_has_produto2` `vp2` LEFT JOIN `estoque_has_consumo` `ehc` ON ((`vp2`.`idVendaProduto2` = `ehc`.`idVendaProduto2`))) WHERE idVenda = '" +
      modelVendas.data(selection.first().row(), "idVenda").toString() + "' GROUP BY `vp2`.`idVendaProduto2`");

  modelProdutos.select();

  modelProdutos.setHeaderData("status", "Status");
  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("idVenda", "Venda");
  modelProdutos.setHeaderData("produto", "Produto");
  modelProdutos.setHeaderData("caixas", "Caixas");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("quantCaixa", "Quant./Cx.");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");
  modelProdutos.setHeaderData("dataRealEnt", "Data Ent.");
  modelProdutos.setHeaderData("obs", "Observação");

  modelProdutos.proxyModel = new SortFilterProxyModel(&modelProdutos, this);

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("dataPrevEnt");
  ui->tableProdutos->hideColumn("idConsumo");
}

void WidgetLogisticaEntregues::on_pushButtonCancelar_clicked() {
  // TODO: se for pedido de representacao voltar status para 'EM ENTREGA' no lugar de 'ESTOQUE', senão não dá para marcar entregue

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  for (const auto &index : list) {
    const QString status = modelProdutos.data(index.row(), "status").toString();

    if (status != "ENTREGUE") { throw RuntimeError("Produto não marcado como 'ENTREGUE'!", this); }
  }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelProdutos.data(index.row(), "idVenda").toString(); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Cancelar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // desfazer passos da confirmacao de entrega (volta para tela de confirmar entrega)

  qApp->startTransaction("WidgetLogisticaEntregues::on_pushButtonCancelar");

  cancelar(list);

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Entrega cancelada!", this);
}

void WidgetLogisticaEntregues::cancelar(const QModelIndexList &list) {
  SqlQuery query1;
  query1.prepare("UPDATE veiculo_has_produto SET status = 'CANCELADO' WHERE `idVendaProduto2` = :idVendaProduto2");

  SqlQuery query2;
  query2.prepare("UPDATE venda_has_produto2 SET status = 'ESTOQUE', entregou = NULL, recebeu = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idVendaProduto2` = :idVendaProduto2 "
                 "AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");

  SqlQuery query3;
  query3.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET status = 'ESTOQUE', dataPrevEnt = NULL, dataRealEnt = NULL WHERE idVendaProduto2 = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  for (const auto &index : list) {
    query1.bindValue(":idVendaProduto2", modelProdutos.data(index.row(), "idVendaProduto2"));

    if (not query1.exec()) { throw RuntimeException("Erro atualizando veiculo_produto: " + query1.lastError().text()); }

    query2.bindValue(":idVendaProduto2", modelProdutos.data(index.row(), "idVendaProduto2"));

    if (not query2.exec()) { throw RuntimeException("Erro atualizando venda_produto: " + query2.lastError().text()); }

    query3.bindValue(":idVendaProduto2", modelProdutos.data(index.row(), "idVendaProduto2"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando pedido_fornecedor: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaEntregues::on_tableVendas_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelVendas.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelVendas.data(index.row(), "idVenda")); }
}

void WidgetLogisticaEntregues::on_tableProdutos_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelProdutos.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelProdutos.data(index.row(), "idVenda")); }
}

// TODO: 0mostrar quem entregou/recebeu nos produtos
