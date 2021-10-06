#include "widgethistoricocompra.h"
#include "ui_widgethistoricocompra.h"

#include "acbrlib.h"
#include "application.h"
#include "doubledelegate.h"
#include "followup.h"
#include "inputdialogfinanceiro.h"
#include "produtoproxymodel.h"
#include "reaisdelegate.h"

#include <QSqlError>

WidgetHistoricoCompra::WidgetHistoricoCompra(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetHistoricoCompra) { ui->setupUi(this); }

WidgetHistoricoCompra::~WidgetHistoricoCompra() { delete ui; }

void WidgetHistoricoCompra::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetHistoricoCompra::on_lineEditBusca_textChanged, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetHistoricoCompra::delayFiltro, connectionType);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetHistoricoCompra::on_pushButtonDanfe_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetHistoricoCompra::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->tablePedidos, &TableView::clicked, this, &WidgetHistoricoCompra::on_tablePedidos_clicked, connectionType);
}

void WidgetHistoricoCompra::updateTables() {
  if (not isSet) {
    timer.setSingleShot(true);
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelViewComprasFinanceiro.select();
}

void WidgetHistoricoCompra::delayFiltro() { timer.start(qApp->delayedTimer); }

void WidgetHistoricoCompra::resetTables() { modelIsSet = false; }

void WidgetHistoricoCompra::setupTables() {
  modelViewComprasFinanceiro.setTable("view_compras_financeiro");

  ui->tablePedidos->setModel(&modelViewComprasFinanceiro);

  ui->tablePedidos->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->tablePedidos->hideColumn("Compra");

  //------------------------------------------------------

  modelProdutos.setTable("pedido_fornecedor_has_produto");

  //------------------------------------------------------

  modelProdutos2.setTable("pedido_fornecedor_has_produto2");

  //------------------------------------------------------

  // TODO: substituir view por query
  modelNFe.setTable("view_ordemcompra_nfe");

  modelNFe.setHeaderData("numeroNFe", "NFe");

  ui->tableNFe->setModel(&modelNFe);

  ui->tableNFe->hideColumn("ordemCompra");
  ui->tableNFe->hideColumn("idNFe");
}

void WidgetHistoricoCompra::setTreeView() {
  modelTree.appendModel(&modelProdutos);
  modelTree.appendModel(&modelProdutos2);

  modelTree.updateData();

  modelTree.setHeaderData("status", "Status");
  modelTree.setHeaderData("statusFinanceiro", "Financeiro");
  modelTree.setHeaderData("ordemRepresentacao", "Cód. Rep.");
  modelTree.setHeaderData("codFornecedor", "Cód. Forn.");
  modelTree.setHeaderData("idVenda", "Venda");
  modelTree.setHeaderData("descricao", "Produto");
  modelTree.setHeaderData("colecao", "Coleção");
  modelTree.setHeaderData("codComercial", "Cód. Com.");
  modelTree.setHeaderData("quant", "Quant.");
  modelTree.setHeaderData("un", "Un.");
  modelTree.setHeaderData("caixas", "Cx.");
  modelTree.setHeaderData("prcUnitario", "R$ Unit.");
  modelTree.setHeaderData("preco", "R$");
  modelTree.setHeaderData("formComercial", "Form. Com.");
  modelTree.setHeaderData("obs", "Obs.");
  modelTree.setHeaderData("dataPrevCompra", "Prev. Compra");
  modelTree.setHeaderData("dataRealCompra", "Compra");
  modelTree.setHeaderData("dataPrevConf", "Prev. Confirmação");
  modelTree.setHeaderData("dataRealConf", "Confirmação");
  modelTree.setHeaderData("dataPrevFat", "Prev. Faturamento");
  modelTree.setHeaderData("dataRealFat", "Faturamento");
  modelTree.setHeaderData("dataPrevColeta", "Prev. Coleta");
  modelTree.setHeaderData("dataRealColeta", "Coleta");
  modelTree.setHeaderData("dataPrevReceb", "Prev. Recebimento");
  modelTree.setHeaderData("dataRealReceb", "Recebimento");
  modelTree.setHeaderData("dataPrevEnt", "Prev. Entrega");
  modelTree.setHeaderData("dataRealEnt", "Entrega");

  modelTree.proxyModel = new ProdutoProxyModel(&modelTree, this);

  ui->treeView->setModel(&modelTree);

  ui->treeView->hideColumn("idRelacionado");
  ui->treeView->hideColumn("selecionado");
  ui->treeView->hideColumn("aliquotaSt");
  ui->treeView->hideColumn("st");
  ui->treeView->hideColumn("ordemCompra");
  ui->treeView->hideColumn("idVendaProduto1");
  ui->treeView->hideColumn("idVendaProduto2");
  ui->treeView->hideColumn("idCompra");
  ui->treeView->hideColumn("fornecedor");
  ui->treeView->hideColumn("idProduto");
  ui->treeView->hideColumn("quantUpd");
  ui->treeView->hideColumn("un2");
  ui->treeView->hideColumn("kgcx");
  ui->treeView->hideColumn("codBarras");
  ui->treeView->hideColumn("created");
  ui->treeView->hideColumn("lastUpdated");

  ui->treeView->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));
  ui->treeView->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("kgcx", new DoubleDelegate(4, this));
}

void WidgetHistoricoCompra::on_tablePedidos_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString ordemCompra = modelViewComprasFinanceiro.data(index.row(), "OC").toString();

  modelProdutos.setFilter("ordemCompra = " + ordemCompra);

  modelProdutos.select();

  modelProdutos2.setFilter("ordemCompra = " + ordemCompra);

  modelProdutos2.select();

  setTreeView();

  modelNFe.setFilter("ordemCompra = " + ordemCompra);

  modelNFe.select();
}

void WidgetHistoricoCompra::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetHistoricoCompra::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = text.isEmpty() ? "0" : "OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%'";

  modelViewComprasFinanceiro.setFilter(filtroBusca);
}

void WidgetHistoricoCompra::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableNFe->selectionModel()->selectedRows();
  const auto rowCount = ui->tableNFe->rowCount();

  if (list.isEmpty() and rowCount != 1) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const int row = (list.isEmpty()) ? 0 : list.first().row();
  const int idNFe = modelNFe.data(row, "idNFe").toInt();

  ACBrLib::gerarDanfe(idNFe);
}

void WidgetHistoricoCompra::on_pushButtonFollowup_clicked() {
  const auto selection = ui->tablePedidos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeException("Nenhuma linha selecionada!"); }

  const QString ordemCompra = modelViewComprasFinanceiro.data(selection.first().row(), "OC").toString();

  auto *followup = new FollowUp(ordemCompra, FollowUp::Tipo::Compra, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

// TODO: 1quando recalcula fluxo deve ter um campo para digitar/calcular ST pois o antigo é substituido e não é criado um novo
// TODO: 4verificar se dá para refazer/ajustar o fluxo de pagamento de acordo com as duplicatas da nota
// TODO: renomear essa classe para WidgetCompraHistorico
// TODO: duplo clique na coluna de idVenda deve abrir a Venda para facilitar
