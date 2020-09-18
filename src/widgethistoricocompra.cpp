#include "widgethistoricocompra.h"
#include "ui_widgethistoricocompra.h"

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "searchdialogproxymodel.h"

#include <QMessageBox>
#include <QSqlError>

WidgetHistoricoCompra::WidgetHistoricoCompra(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetHistoricoCompra) { ui->setupUi(this); }

WidgetHistoricoCompra::~WidgetHistoricoCompra() { delete ui; }

void WidgetHistoricoCompra::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetHistoricoCompra::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetHistoricoCompra::on_pushButtonDanfe_clicked, connectionType);
  connect(ui->tablePedidos, &TableView::clicked, this, &WidgetHistoricoCompra::on_tablePedidos_clicked, connectionType);
}

void WidgetHistoricoCompra::updateTables() {
  if (not isSet) {
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

  ui->tableNFe->hideColumn("idCompra");
  ui->tableNFe->hideColumn("ordemCompra");
  ui->tableNFe->hideColumn("idNFe");
}

void WidgetHistoricoCompra::setTreeView() {
  modelTree.appendModel(&modelProdutos);
  modelTree.appendModel(&modelProdutos2);

  modelTree.updateData();

  modelTree.setHeaderData("status", "Status");
  modelTree.setHeaderData("statusFinanceiro", "Financeiro");
  modelTree.setHeaderData("ordemRepresentacao", "OC Rep");
  modelTree.setHeaderData("idVenda", "Venda");
  modelTree.setHeaderData("descricao", "Produto");
  modelTree.setHeaderData("colecao", "Coleção");
  modelTree.setHeaderData("codComercial", "Cód. Com.");
  modelTree.setHeaderData("quant", "Quant.");
  modelTree.setHeaderData("un", "Un.");
  modelTree.setHeaderData("un2", "Un2.");
  modelTree.setHeaderData("caixas", "Cx.");
  modelTree.setHeaderData("prcUnitario", "R$ Unit.");
  modelTree.setHeaderData("preco", "R$");
  modelTree.setHeaderData("kgcx", "Kg./Cx.");
  modelTree.setHeaderData("formComercial", "Form. Com.");
  modelTree.setHeaderData("codBarras", "Cód. Barras");
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

  modelTree.proxyModel = new SearchDialogProxyModel(&modelTree, this);

  ui->treeView->setModel(&modelTree);

  ui->treeView->hideColumn("codFornecedor");
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

void WidgetHistoricoCompra::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetHistoricoCompra::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = text.isEmpty() ? "" : "OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%'";

  modelViewComprasFinanceiro.setFilter(filtroBusca);
}

void WidgetHistoricoCompra::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableNFe->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  ACBr acbrLocal;
  acbrLocal.gerarDanfe(modelNFe.data(list.first().row(), "idNFe").toInt());
}

// TODO: 1quando recalcula fluxo deve ter um campo para digitar/calcular ST pois o antigo é substituido e não é criado um novo
// TODO: 4verificar se dá para refazer/ajustar o fluxo de pagamento de acordo com as duplicatas da nota
