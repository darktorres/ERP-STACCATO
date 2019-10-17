#include <QDebug>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>

#include "application.h"
#include "doubledelegate.h"
#include "sortfilterproxymodel.h"
#include "ui_widgetlogisticaentregues.h"
#include "usersession.h"
#include "vendaproxymodel.h"
#include "widgetlogisticaentregues.h"

WidgetLogisticaEntregues::WidgetLogisticaEntregues(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaEntregues) { ui->setupUi(this); }

WidgetLogisticaEntregues::~WidgetLogisticaEntregues() { delete ui; }

void WidgetLogisticaEntregues::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaEntregues::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->radioButtonEntregaLimpar, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->radioButtonParcialEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->radioButtonSemEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->radioButtonTotalEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro, connectionType);
  connect(ui->tableVendas, &TableView::clicked, this, &WidgetLogisticaEntregues::on_tableVendas_clicked, connectionType);
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

  modelVendas.setSort("prazoEntrega");

  modelVendas.setHeaderData("idVenda", "Venda");
  modelVendas.setHeaderData("prazoEntrega", "Prazo Limite");

  ui->tableVendas->setModel(&modelVendas);

  ui->tableVendas->setItemDelegate(new DoubleDelegate(this));
}

void WidgetLogisticaEntregues::on_tableVendas_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  modelProdutos.setQuery(
      "SELECT `vp2`.`idVendaProduto2` AS `idVendaProduto2`, `vp2`.`idProduto` AS `idProduto`, `vp2`.`dataPrevEnt` AS `dataPrevEnt`, `vp2`.`dataRealEnt` AS `dataRealEnt`, `vp2`.`status` "
      "AS `status`, `vp2`.`fornecedor` AS `fornecedor`, `vp2`.`idVenda` AS `idVenda`, `vp2`.`produto` AS `produto`, `vp2`.`caixas` AS `caixas`, `vp2`.`quant` AS `quant`, `vp2`.`un` AS "
      "`un`, `vp2`.`unCaixa` AS `unCaixa`, `vp2`.`codComercial` AS `codComercial`, `vp2`.`formComercial` AS `formComercial`, GROUP_CONCAT(DISTINCT`ehc`.`idConsumo`) AS `idConsumo` "
      "FROM (`venda_has_produto2` `vp2` LEFT JOIN `estoque_has_consumo` `ehc` ON ((`vp2`.`idVendaProduto2` = `ehc`.`idVendaProduto2`))) WHERE idVenda = '" +
      modelVendas.data(index.row(), "idVenda").toString() + "' GROUP BY `vp2`.`idVendaProduto2`");

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

  ui->tableProdutos->setModel(new SortFilterProxyModel(&modelProdutos, this));

  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("dataPrevEnt");
  ui->tableProdutos->hideColumn("idConsumo");
}

void WidgetLogisticaEntregues::on_pushButtonCancelar_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  for (const auto &index : list) {
    const QString status = modelProdutos.data(index.row(), "status").toString();

    if (status != "ENTREGUE") { return qApp->enqueueError("Produto não marcado como 'ENTREGUE'!", this); }
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // desfazer passos da confirmacao de entrega (volta para tela de confirmar entrega)

  if (not qApp->startTransaction()) { return; }

  if (not cancelar(list)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Entrega cancelada!", this);
}

bool WidgetLogisticaEntregues::cancelar(const QModelIndexList &list) {
  // FIXME: ao voltar vp para ESTOQUE permite que o usuario eventualmente gere outra nota e a primeira vai ficar perdida no limbo

  QSqlQuery query1;
  query1.prepare("UPDATE veiculo_has_produto SET status = 'CANCELADO' WHERE `idVendaProduto2` = :idVendaProduto2");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto2 SET status = 'ESTOQUE', entregou = NULL, recebeu = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE `idVendaProduto2` = :idVendaProduto2 "
                 "AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");

  QSqlQuery query3;
  query3.prepare(
      "UPDATE pedido_fornecedor_has_produto SET status = 'ESTOQUE', dataPrevEnt = NULL, dataRealEnt = NULL WHERE idVendaProduto = :idVendaProduto AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");

  for (const auto &index : list) {
    query1.bindValue(":idVendaProduto2", modelProdutos.data(index.row(), "idVendaProduto2"));

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro atualizando veiculo_produto: " + query1.lastError().text(), this); }

    query2.bindValue(":idVendaProduto2", modelProdutos.data(index.row(), "idVendaProduto2"));

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando venda_produto: " + query2.lastError().text(), this); }

    query3.bindValue(":idVendaProduto", modelProdutos.data(index.row(), "idVendaProduto"));

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro atualizando pedido_fornecedor: " + query3.lastError().text(), this); }
  }

  return true;
}

// TODO: 0mostrar quem entregou/recebeu
