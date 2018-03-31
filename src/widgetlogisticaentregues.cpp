#include <QDebug>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>

#include "doubledelegate.h"
#include "sql.h"
#include "ui_widgetlogisticaentregues.h"
#include "usersession.h"
#include "vendaproxymodel.h"
#include "widgetlogisticaentregues.h"

WidgetLogisticaEntregues::WidgetLogisticaEntregues(QWidget *parent) : Widget(parent), ui(new Ui::WidgetLogisticaEntregues) {
  ui->setupUi(this);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &WidgetLogisticaEntregues::on_pushButtonCancelar_clicked);
  connect(ui->tableVendas, &TableView::clicked, this, &WidgetLogisticaEntregues::on_tableVendas_clicked);
}

WidgetLogisticaEntregues::~WidgetLogisticaEntregues() { delete ui; }

bool WidgetLogisticaEntregues::updateTables() {
  if (modelVendas.tableName().isEmpty()) {
    setupTables();

    connect(ui->radioButtonEntregaLimpar, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
    connect(ui->radioButtonParcialEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
    connect(ui->radioButtonSemEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
    connect(ui->radioButtonTotalEntrega, &QRadioButton::clicked, this, &WidgetLogisticaEntregues::montaFiltro);
    connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaEntregues::montaFiltro);
  }

  if (not modelVendas.select()) {
    emit errorSignal("Erro lendo tabela vendas: " + modelVendas.lastError().text());
    return false;
  }

  ui->tableVendas->sortByColumn("prazoEntrega");

  ui->tableVendas->resizeColumnsToContents();

  modelProdutos.setQuery(modelProdutos.query().executedQuery());

  if (modelProdutos.lastError().isValid()) {
    emit errorSignal("Erro lendo tabela produtos: " + modelProdutos.lastError().text());
    return false;
  }

  ui->tableProdutos->resizeColumnsToContents();

  return true;
}

void WidgetLogisticaEntregues::montaFiltro() {
  QString filtroCheck;

  if (ui->radioButtonEntregaLimpar->isChecked()) filtroCheck = "";
  if (ui->radioButtonTotalEntrega->isChecked()) filtroCheck = "Entregue > 0 AND Estoque = 0 AND Outros = 0";
  if (ui->radioButtonParcialEntrega->isChecked()) filtroCheck = "Entregue > 0 AND (Estoque > 0 OR Outros > 0)";
  if (ui->radioButtonSemEntrega->isChecked()) filtroCheck = "Entregue = 0";

  const QString textoBusca = ui->lineEditBusca->text();

  QString filtroBusca = textoBusca.isEmpty() ? "" : "idVenda LIKE '%" + textoBusca + "%'";

  if (not filtroCheck.isEmpty() and not filtroBusca.isEmpty()) filtroBusca.prepend(" AND ");

  modelVendas.setFilter(filtroCheck + filtroBusca);

  ui->tableVendas->resizeColumnsToContents();
}

void WidgetLogisticaEntregues::setupTables() {
  modelVendas.setTable("view_entrega");
  modelVendas.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelVendas.setHeaderData("idVenda", "Venda");
  modelVendas.setHeaderData("prazoEntrega", "Prazo Limite");

  ui->tableVendas->setModel(new VendaProxyModel(&modelVendas, this));
  ui->tableVendas->setItemDelegate(new DoubleDelegate(this));

  if (UserSession::tipoUsuario() != "VENDEDOR ESPECIAL") ui->tableVendas->hideColumn("Indicou");
}

void WidgetLogisticaEntregues::on_tableVendas_clicked(const QModelIndex &index) {
  modelProdutos.setQuery("SELECT `vp`.`idVendaProduto` AS `idVendaProduto`, `vp`.`idProduto` AS `idProduto`, `vp`.`dataPrevEnt` AS `dataPrevEnt`, `vp`.`dataRealEnt` AS `dataRealEnt`, `vp`.`status` "
                         "AS `status`, `vp`.`fornecedor` AS `fornecedor`, `vp`.`idVenda` AS `idVenda`, `vp`.`produto` AS `produto`, `vp`.`caixas` AS `caixas`, `vp`.`quant` AS `quant`, `vp`.`un` AS "
                         "`un`, `vp`.`unCaixa` AS `unCaixa`, `vp`.`codComercial` AS `codComercial`, `vp`.`formComercial` AS `formComercial`, GROUP_CONCAT(DISTINCT`ehc`.`idConsumo`) AS `idConsumo` "
                         "FROM (`mydb`.`venda_has_produto` `vp` LEFT JOIN `mydb`.`estoque_has_consumo` `ehc` ON ((`vp`.`idVendaProduto` = `ehc`.`idVendaProduto`))) WHERE idVenda = '" +
                         modelVendas.data(index.row(), "idVenda").toString() + "' GROUP BY `vp`.`idVendaProduto`");

  if (modelProdutos.lastError().isValid()) {
    emit errorSignal("Erro: " + modelProdutos.lastError().text());
    return;
  }

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

  auto *proxyFilter = new QSortFilterProxyModel(this);
  proxyFilter->setDynamicSortFilter(true);
  proxyFilter->setSourceModel(&modelProdutos);

  ui->tableProdutos->setModel(proxyFilter);
  ui->tableProdutos->hideColumn("idVendaProduto");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("dataPrevEnt");

  for (int row = 0; row < modelProdutos.rowCount(); ++row) ui->tableProdutos->openPersistentEditor(row, "selecionado");

  ui->tableProdutos->resizeColumnsToContents();
}

void WidgetLogisticaEntregues::on_pushButtonCancelar_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhuma linha selecionada!");
    return;
  }

  QStringList idVendas;

  for (const auto &index : list) idVendas << modelProdutos.data(index.row(), "idVenda").toString();

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // desfazer passos da confirmacao de entrega (volta para tela de confirmar entrega)

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not cancelar(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not Sql::updateVendaStatus(idVendas.join(", "))) { return; }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Entrega cancelada!");
}

bool WidgetLogisticaEntregues::cancelar(const QModelIndexList &list) {
  QSqlQuery query1;
  query1.prepare("UPDATE veiculo_has_produto SET status = 'CANCELADO' WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET status = 'ESTOQUE', entregou = NULL, recebeu = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE idVendaProduto = :idVendaProduto");

  for (const auto &item : list) {
    query1.bindValue(":idVendaProduto", modelProdutos.data(item.row(), "idVendaProduto"));

    if (not query1.exec()) {
      emit errorSignal("Erro atualizando veiculo_produto: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":idVendaProduto", modelProdutos.data(item.row(), "idVendaProduto"));

    if (not query2.exec()) {
      emit errorSignal("Erro atualizando venda_produto: " + query2.lastError().text());
      return false;
    }
  }

  return true;
}

// TODO: 0mostrar quem entregou/recebeu
