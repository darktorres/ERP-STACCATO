#include <QDebug>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>

#include "application.h"
#include "doubledelegate.h"
#include "sortfilterproxymodel.h"
#include "sql.h"
#include "ui_widgetlogisticaentregues.h"
#include "usersession.h"
#include "vendaproxymodel.h"
#include "widgetlogisticaentregues.h"

WidgetLogisticaEntregues::WidgetLogisticaEntregues(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaEntregues) { ui->setupUi(this); }

WidgetLogisticaEntregues::~WidgetLogisticaEntregues() { delete ui; }

void WidgetLogisticaEntregues::setConnections() {
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaEntregues::montaFiltro);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaEntregues::on_pushButtonCancelar_clicked);
  connect(ui->radioButtonEntregaLimpar, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
  connect(ui->radioButtonParcialEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
  connect(ui->radioButtonSemEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
  connect(ui->radioButtonTotalEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
  connect(ui->tableVendas, &TableView::clicked, this, &WidgetLogisticaEntregues::on_tableVendas_clicked);
}

void WidgetLogisticaEntregues::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelVendas.select()) { return; }

  // -----------------------------------------------------------------

  modelProdutos.setQuery(modelProdutos.query().executedQuery());

  if (modelProdutos.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela produtos: " + modelProdutos.lastError().text(), this); }
}

void WidgetLogisticaEntregues::resetTables() { modelIsSet = false; }

void WidgetLogisticaEntregues::montaFiltro() {
  QStringList filtros;

  QString filtroCheck;

  if (ui->radioButtonEntregaLimpar->isChecked()) { filtroCheck = ""; }
  if (ui->radioButtonTotalEntrega->isChecked()) { filtroCheck = "Entregue > 0 AND Estoque = 0 AND Outros = 0"; }
  if (ui->radioButtonParcialEntrega->isChecked()) { filtroCheck = "Entregue > 0 AND (Estoque > 0 OR Outros > 0)"; }
  if (ui->radioButtonSemEntrega->isChecked()) { filtroCheck = "Entregue = 0"; }

  if (not filtroCheck.isEmpty()) { filtros << filtroCheck; }

  //-------------------------------------

  const QString textoBusca = ui->lineEditBusca->text();
  const QString filtroBusca = "idVenda LIKE '%" + textoBusca + "%'";
  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelVendas.setFilter(filtros.join(" AND "));
}

void WidgetLogisticaEntregues::setupTables() {
  modelVendas.setTable("view_entrega");

  modelVendas.setSort("prazoEntrega", Qt::AscendingOrder);

  modelVendas.setHeaderData("idVenda", "Venda");
  modelVendas.setHeaderData("prazoEntrega", "Prazo Limite");

  ui->tableVendas->setModel(&modelVendas);

  ui->tableVendas->setItemDelegate(new DoubleDelegate(this));
}

void WidgetLogisticaEntregues::on_tableVendas_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  modelProdutos.setQuery("SELECT `vp`.`idVendaProduto` AS `idVendaProduto`, `vp`.`idProduto` AS `idProduto`, `vp`.`dataPrevEnt` AS `dataPrevEnt`, `vp`.`dataRealEnt` AS `dataRealEnt`, `vp`.`status` "
                         "AS `status`, `vp`.`fornecedor` AS `fornecedor`, `vp`.`idVenda` AS `idVenda`, `vp`.`produto` AS `produto`, `vp`.`caixas` AS `caixas`, `vp`.`quant` AS `quant`, `vp`.`un` AS "
                         "`un`, `vp`.`unCaixa` AS `unCaixa`, `vp`.`codComercial` AS `codComercial`, `vp`.`formComercial` AS `formComercial`, GROUP_CONCAT(DISTINCT`ehc`.`idConsumo`) AS `idConsumo` "
                         "FROM (`venda_has_produto` `vp` LEFT JOIN `estoque_has_consumo` `ehc` ON ((`vp`.`idVendaProduto` = `ehc`.`idVendaProduto`))) WHERE idVenda = '" +
                         modelVendas.data(index.row(), "idVenda").toString() + "' GROUP BY `vp`.`idVendaProduto`");

  if (modelProdutos.lastError().isValid()) { return qApp->enqueueError("Erro: " + modelProdutos.lastError().text(), this); }

  modelProdutos.setHeaderData("status", "Status");
  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("idVenda", "Venda");
  modelProdutos.setHeaderData("produto", "Produto");
  modelProdutos.setHeaderData("caixas", "Caixas");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("unCaixa", "Un./Cx.");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");
  modelProdutos.setHeaderData("dataRealEnt", "Data Ent.");

  modelProdutos.proxyModel = new SortFilterProxyModel(&modelProdutos, this);

  ui->tableProdutos->setModel(&modelProdutos);
  ui->tableProdutos->hideColumn("idVendaProduto");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("dataPrevEnt");
  ui->tableProdutos->hideColumn("idConsumo");

  ui->tableProdutos->setPersistentColumns({"selecionado"});
}

void WidgetLogisticaEntregues::on_pushButtonCancelar_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  for (const auto &index : list) {
    const QString status = modelProdutos.data(index.row(), "status").toString();

    if (status != "ENTREGUE") { return qApp->enqueueError("Produto não marcado como 'ENTREGUE'!", this); }
  }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelProdutos.data(index.row(), "idVenda").toString(); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // desfazer passos da confirmacao de entrega (volta para tela de confirmar entrega)

  if (not qApp->startTransaction()) { return; }

  if (not cancelar(list)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVendas)) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Entrega cancelada!", this);
}

bool WidgetLogisticaEntregues::cancelar(const QModelIndexList &list) {
  QSqlQuery query1;
  query1.prepare("UPDATE veiculo_has_produto SET status = 'CANCELADO' WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET status = 'ESTOQUE', entregou = NULL, recebeu = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE idVendaProduto = :idVendaProduto "
                 "AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  for (const auto &item : list) {
    query1.bindValue(":idVendaProduto", modelProdutos.data(item.row(), "idVendaProduto"));

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro atualizando veiculo_produto: " + query1.lastError().text(), this); }

    query2.bindValue(":idVendaProduto", modelProdutos.data(item.row(), "idVendaProduto"));

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando venda_produto: " + query2.lastError().text(), this); }
  }

  return true;
}

// TODO: 0mostrar quem entregou/recebeu
