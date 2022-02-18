#include "widgetlogisticarepresentacao.h"
#include "ui_widgetlogisticarepresentacao.h"

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "followup.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

WidgetLogisticaRepresentacao::WidgetLogisticaRepresentacao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaRepresentacao) { ui->setupUi(this); }

WidgetLogisticaRepresentacao::~WidgetLogisticaRepresentacao() { delete ui; }

void WidgetLogisticaRepresentacao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetLogisticaRepresentacao::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetLogisticaRepresentacao::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonLimparFiltro, &QPushButton::clicked, this, &WidgetLogisticaRepresentacao::on_pushButtonLimparFiltro_clicked, connectionType);
  connect(ui->pushButtonMarcarEntregue, &QPushButton::clicked, this, &WidgetLogisticaRepresentacao::on_pushButtonMarcarEntregue_clicked, connectionType);
  connect(ui->table, &QTableView::doubleClicked, this, &WidgetLogisticaRepresentacao::on_table_doubleClicked, connectionType);
  connect(ui->tableForn->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaRepresentacao::on_tableForn_selectionChanged, connectionType);
}

void WidgetLogisticaRepresentacao::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    setupTables();
    setConnections();
    isSet = true;
  }

  modelRepresentacao.select();
  modelFornecedor.select();
}

void WidgetLogisticaRepresentacao::resetTables() { setupTables(); }

void WidgetLogisticaRepresentacao::setupTables() {
  modelRepresentacao.setTable("view_logistica_representacao");

  modelRepresentacao.setFilter("");

  modelRepresentacao.setHeaderData("idVenda", "Venda");
  modelRepresentacao.setHeaderData("cliente", "Cliente");
  modelRepresentacao.setHeaderData("descricao", "Produto");
  modelRepresentacao.setHeaderData("codComercial", "Cód. Com.");
  modelRepresentacao.setHeaderData("quant", "Quant.");
  modelRepresentacao.setHeaderData("un", "Un.");
  modelRepresentacao.setHeaderData("caixas", "Cx.");
  modelRepresentacao.setHeaderData("kgcx", "Kg./Cx.");
  modelRepresentacao.setHeaderData("ordemCompra", "O.C.");
  modelRepresentacao.setHeaderData("prazoEntrega", "Prazo Limite");

  modelRepresentacao.setSort("prazoEntrega");

  modelRepresentacao.proxyModel = new EstoquePrazoProxyModel(&modelRepresentacao, this);

  ui->table->setModel(&modelRepresentacao);

  ui->table->hideColumn("idPedido2");
  ui->table->hideColumn("fornecedor");
  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("idCompra");

  // -------------------------------------------------------------------------

  modelFornecedor.setTable("view_fornecedor_logistica_representacao");

  modelFornecedor.setFilter("");

  ui->tableForn->setModel(&modelFornecedor);

  ui->tableForn->sortByColumn("Fornecedor");
}

void WidgetLogisticaRepresentacao::on_pushButtonMarcarEntregue_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList idVendas;

  for (const auto &index : selection) { idVendas << modelRepresentacao.data(index.row(), "idVenda").toString(); }

  InputDialogConfirmacao input(InputDialogConfirmacao::Tipo::Representacao, this);

  if (input.exec() != InputDialogConfirmacao::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaRepresentacao::on_pushButtonMarcarEntregue");

  processRows(selection, input.getDate(), input.getRecebeu());

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  ui->table->clearSelection();

  updateTables();
  qApp->enqueueInformation("Entrega confirmada!", this);
}

void WidgetLogisticaRepresentacao::processRows(const QModelIndexList &list, const QDate dataEntrega, const QString &recebeu) {
  SqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'ENTREGUE', dataRealEnt = :dataRealEnt WHERE status = 'EM ENTREGA' AND idVendaProduto2 = :idVendaProduto2");

  SqlQuery query2;
  query2.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGUE', dataRealEnt = :dataRealEnt, recebeu = :recebeu WHERE status = 'EM ENTREGA' AND idVendaProduto2 = :idVendaProduto2");

  for (const auto &index : list) {
    query1.bindValue(":dataRealEnt", dataEntrega);
    query1.bindValue(":idVendaProduto2", modelRepresentacao.data(index.row(), "idVendaProduto2"));

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":dataRealEnt", dataEntrega);
    query2.bindValue(":idVendaProduto2", modelRepresentacao.data(index.row(), "idVendaProduto2"));
    query2.bindValue(":recebeu", recebeu);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query2.lastError().text()); }
  }
}

void WidgetLogisticaRepresentacao::on_lineEditBusca_textChanged() {
  const QString text = ui->lineEditBusca->text();

  modelRepresentacao.setFilter("(idVenda LIKE '%" + text + "%' OR cliente LIKE '%" + text + "%')");
}

void WidgetLogisticaRepresentacao::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = modelRepresentacao.data(selection.first().row(), "idVenda").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetLogisticaRepresentacao::on_table_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelRepresentacao.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelRepresentacao.data(index.row(), "idVenda")); }

  if (header == "O.C.") { return qApp->abrirCompra(modelRepresentacao.data(index.row(), "ordemCompra")); }
}

void WidgetLogisticaRepresentacao::on_pushButtonLimparFiltro_clicked() {
  ui->table->clearSelection();
  ui->tableForn->clearSelection();
}

void WidgetLogisticaRepresentacao::on_tableForn_selectionChanged() {
  const auto selection = ui->tableForn->selectionModel()->selectedRows();

  ui->lineEditBusca->clear();

  const QString fornecedor = (selection.isEmpty()) ? "" : modelFornecedor.data(selection.first().row(), "fornecedor").toString();

  const QString filtro = (fornecedor.isEmpty()) ? "" : "fornecedor = '" + fornecedor + "'";

  modelRepresentacao.setFilter(filtro);
}

// TODO: 2palimanan precisa de coleta/recebimento (colocar flag no cadastro dizendo que entra no fluxo de logistica)
// TODO: colocar botao para cancelar
