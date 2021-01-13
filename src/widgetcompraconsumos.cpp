#include "widgetcompraconsumos.h"
#include "ui_widgetcompraconsumos.h"

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "reaisdelegate.h"
#include "sql.h"

#include <QMessageBox>
#include <QSqlError>

WidgetCompraConsumos::WidgetCompraConsumos(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraConsumos) {
  ui->setupUi(this);
  timer.setSingleShot(true);
}

WidgetCompraConsumos::~WidgetCompraConsumos() { delete ui; }

void WidgetCompraConsumos::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelPedido.select();

  modelProduto.select();
}

void WidgetCompraConsumos::delayFiltro() { timer.start(500); }

void WidgetCompraConsumos::resetTables() { modelIsSet = false; }

void WidgetCompraConsumos::setupTables() {
  modelPedido.setTable("view_ordemcompra_resumo");

  ui->tablePedido->setModel(&modelPedido);

  //------------------------------------------------------

  modelProduto.setTable("view_ordemcompra");

  modelProduto.setHeaderData("status", "Status");
  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("produto", "Produto");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("quant", "Quant.");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("caixas", "Cx.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("obs", "Obs.");

  ui->tableProduto->setModel(&modelProduto);

  ui->tableProduto->setItemDelegateForColumn("quant", new DoubleDelegate(this));

  ui->tableProduto->hideColumn("idVenda");
  ui->tableProduto->hideColumn("idVendaProduto2");
}

void WidgetCompraConsumos::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetCompraConsumos::on_lineEditBusca_textChanged, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraConsumos::delayFiltro, connectionType);
  connect(ui->pushButtonDesfazerConsumo, &QPushButton::clicked, this, &WidgetCompraConsumos::on_pushButtonDesfazerConsumo_clicked, connectionType);
  connect(ui->tablePedido, &TableView::clicked, this, &WidgetCompraConsumos::on_tablePedido_clicked, connectionType);
}

void WidgetCompraConsumos::on_tablePedido_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString idVenda = modelPedido.data(index.row(), "Venda").toString();

  modelProduto.setFilter("idVenda = '" + idVenda + "'");
}

void WidgetCompraConsumos::on_pushButtonDesfazerConsumo_clicked() {
  const auto list = ui->tableProduto->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  //------------------------------------

  const int row = list.first().row();

  const QString status = modelProduto.data(row, "status").toString();

  if (status == "PENDENTE" or status == "REPO. ENTREGA" or status == "CANCELADO") { throw RuntimeError("Produto ainda não foi comprado!", this); }

  if (status == "ENTREGA AGEND." or status == "EM ENTREGA" or status == "ENTREGUE") { throw RuntimeError("Produto está em entrega/entregue!", this); }

  if (status == "DEVOLVIDO" or status == "QUEBRADO" or status == "CANCELADO") { throw RuntimeError("Não permitido!", this); }
  //------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Desfazer consumo/Desvincular da compra?", "Tem certeza?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Continuar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  //------------------------------------

  const QString idVenda = modelProduto.data(row, "idVenda").toString();

  qApp->startTransaction("WidgetCompraConsumos::on_pushButtonDesfazerConsumo");

  desfazerConsumo(row);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  //------------------------------------

  updateTables();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void WidgetCompraConsumos::desfazerConsumo(const int row) {
  const int idVendaProduto2 = modelProduto.data(row, "idVendaProduto2").toInt();

  Estoque::desfazerConsumo(idVendaProduto2);
}

void WidgetCompraConsumos::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetCompraConsumos::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  modelPedido.setFilter("Venda LIKE '%" + text + "%' OR OC LIKE '%" + text + "%'");
}

// TODO: converter tabela inferior para arvore e permitir o desconsumo apenas das sublinhas (vp2)
// TODO: ao selecionar uma linha na tabela superior mostrar apenas os produtos da dupla idVenda/idCompra ou juntar todos os idCompra com um group_concat
// TODO: refazer o agrupamento porque tem venda que aparece 3 vezes em cima e nos 3 casos mostra os mesmos itens em baixo (GABR-163619)
