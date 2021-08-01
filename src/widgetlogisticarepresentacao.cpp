#include "widgetlogisticarepresentacao.h"
#include "ui_widgetlogisticarepresentacao.h"

#include "application.h"
#include "estoqueprazoproxymodel.h"
#include "followup.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDate>
#include <QDebug>
#include <QSqlError>

WidgetLogisticaRepresentacao::WidgetLogisticaRepresentacao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaRepresentacao) { ui->setupUi(this); }

WidgetLogisticaRepresentacao::~WidgetLogisticaRepresentacao() { delete ui; }

void WidgetLogisticaRepresentacao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetLogisticaRepresentacao::on_lineEditBusca_textChanged, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaRepresentacao::delayFiltro, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetLogisticaRepresentacao::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonMarcarEntregue, &QPushButton::clicked, this, &WidgetLogisticaRepresentacao::on_pushButtonMarcarEntregue_clicked, connectionType);
}

void WidgetLogisticaRepresentacao::updateTables() {
  if (not isSet) {
    timer.setSingleShot(true);
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  modelViewLogisticaRepresentacao.select();
}

void WidgetLogisticaRepresentacao::delayFiltro() { timer.start(qApp->delayedTimer); }

void WidgetLogisticaRepresentacao::tableFornLogistica_clicked(const QString &fornecedor) {
  ui->lineEditBusca->clear();

  const QString filtro = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";

  modelViewLogisticaRepresentacao.setFilter(filtro);
}

void WidgetLogisticaRepresentacao::resetTables() { modelIsSet = false; }

void WidgetLogisticaRepresentacao::setupTables() {
  modelViewLogisticaRepresentacao.setTable("view_logistica_representacao");

  modelViewLogisticaRepresentacao.setFilter("");

  modelViewLogisticaRepresentacao.setHeaderData("idVenda", "Venda");
  modelViewLogisticaRepresentacao.setHeaderData("cliente", "Cliente");
  modelViewLogisticaRepresentacao.setHeaderData("descricao", "Produto");
  modelViewLogisticaRepresentacao.setHeaderData("codComercial", "Cód. Com.");
  modelViewLogisticaRepresentacao.setHeaderData("quant", "Quant.");
  modelViewLogisticaRepresentacao.setHeaderData("un", "Un.");
  modelViewLogisticaRepresentacao.setHeaderData("caixas", "Cx.");
  modelViewLogisticaRepresentacao.setHeaderData("kgcx", "Kg./Cx.");
  modelViewLogisticaRepresentacao.setHeaderData("ordemCompra", "OC");
  modelViewLogisticaRepresentacao.setHeaderData("prazoEntrega", "Prazo Limite");

  modelViewLogisticaRepresentacao.setSort("prazoEntrega");

  modelViewLogisticaRepresentacao.proxyModel = new EstoquePrazoProxyModel(&modelViewLogisticaRepresentacao, this);

  ui->table->setModel(&modelViewLogisticaRepresentacao);

  ui->table->hideColumn("idPedido2");
  ui->table->hideColumn("fornecedor");
  ui->table->hideColumn("idVendaProduto2");
}

void WidgetLogisticaRepresentacao::on_pushButtonMarcarEntregue_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelViewLogisticaRepresentacao.data(index.row(), "idVenda").toString(); }

  InputDialogConfirmacao input(InputDialogConfirmacao::Tipo::Representacao, this);

  if (input.exec() != InputDialogConfirmacao::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaRepresentacao::on_pushButtonMarcarEntregue");

  processRows(list, input.getDate(), input.getRecebeu());

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

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
    query1.bindValue(":idVendaProduto2", modelViewLogisticaRepresentacao.data(index.row(), "idVendaProduto2"));

    if (not query1.exec()) { throw RuntimeException("Erro salvando status no pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":dataRealEnt", dataEntrega);
    query2.bindValue(":idVendaProduto2", modelViewLogisticaRepresentacao.data(index.row(), "idVendaProduto2"));
    query2.bindValue(":recebeu", recebeu);

    if (not query2.exec()) { throw RuntimeException("Erro salvando status na venda_produto: " + query2.lastError().text()); }
  }
}

void WidgetLogisticaRepresentacao::on_lineEditBusca_textChanged() {
  const QString text = ui->lineEditBusca->text();

  modelViewLogisticaRepresentacao.setFilter("(idVenda LIKE '%" + text + "%' OR cliente LIKE '%" + text + "%')");
}

void WidgetLogisticaRepresentacao::on_pushButtonFollowup_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = modelViewLogisticaRepresentacao.data(list.first().row(), "idVenda").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

// TODO: 2palimanan precisa de coleta/recebimento (colocar flag no cadastro dizendo que entra no fluxo de logistica)
// TODO: colocar botao para cancelar
