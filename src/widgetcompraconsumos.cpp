#include "widgetcompraconsumos.h"
#include "ui_widgetcompraconsumos.h"

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "sql.h"

#include <QMessageBox>
#include <QSqlError>

WidgetCompraConsumos::WidgetCompraConsumos(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraConsumos) { ui->setupUi(this); }

WidgetCompraConsumos::~WidgetCompraConsumos() { delete ui; }

void WidgetCompraConsumos::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelPedido.select();

  modelProduto.select();
}

void WidgetCompraConsumos::resetTables() {
  setupTables();
  montaFiltro();
}

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

  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetCompraConsumos::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonDesfazerConsumo, &QPushButton::clicked, this, &WidgetCompraConsumos::on_pushButtonDesfazerConsumo_clicked, connectionType);
  connect(ui->tablePedido, &TableView::doubleClicked, this, &WidgetCompraConsumos::on_tablePedido_doubleClicked, connectionType);
  connect(ui->tablePedido->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetCompraConsumos::on_tablePedido_selectionChanged, connectionType);
}

void WidgetCompraConsumos::on_tablePedido_selectionChanged() {
  const auto selection = ui->tablePedido->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  const QString idVenda = modelPedido.data(selection.first().row(), "Venda").toString();

  modelProduto.setFilter("idVenda = '" + idVenda + "'");
}

void WidgetCompraConsumos::on_pushButtonDesfazerConsumo_clicked() {
  const auto selection = ui->tableProduto->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  //------------------------------------

  for (const auto &index : selection) {
    const int row = index.row();

    const QString status = modelProduto.data(row, "status").toString();

    if (status == "PENDENTE" or status == "REPO. ENTREGA") { throw RuntimeError("Produto ainda não foi comprado!", this); }

    if (status == "ENTREGA AGEND." or status == "EM ENTREGA" or status == "ENTREGUE") { throw RuntimeError("Produto está em entrega/entregue!", this); }

    if (status == "DEVOLVIDO" or status == "QUEBRADO" or status == "CANCELADO") { throw RuntimeError("Não permitido!", this); }
  }

  //------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Desfazer consumo/Desvincular da compra?", "Tem certeza?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Continuar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  //------------------------------------

  const QString idVenda = modelProduto.data(selection.first().row(), "idVenda").toString();

  qApp->startTransaction("WidgetCompraConsumos::on_pushButtonDesfazerConsumo");

  desfazerConsumo(selection);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  //------------------------------------

  updateTables();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void WidgetCompraConsumos::desfazerConsumo(const QModelIndexList &list) {
  for (const auto &index : list) {
    const int idVendaProduto2 = modelProduto.data(index.row(), "idVendaProduto2").toInt();
    Estoque::desfazerConsumo(idVendaProduto2);
  }
}

void WidgetCompraConsumos::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetCompraConsumos::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = text.isEmpty() ? "0" : "(Venda LIKE '%" + text + "%' OR OC LIKE '%" + text + "%')";

  modelPedido.setFilter(filtroBusca);
}

void WidgetCompraConsumos::on_tablePedido_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelPedido.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelPedido.data(index.row(), "Venda")); }
}

// TODO: converter tabela inferior para arvore e permitir o desconsumo apenas das sublinhas (vp2)
// TODO: ao selecionar uma linha na tabela superior mostrar apenas os produtos da dupla idVenda/idCompra ou juntar todos os idCompra com um group_concat
// TODO: refazer o agrupamento porque tem venda que aparece 3 vezes em cima e nos 3 casos mostra os mesmos itens em baixo (GABR-163619)
