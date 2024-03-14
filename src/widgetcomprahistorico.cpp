#include "widgetcomprahistorico.h"
#include "ui_widgetcomprahistorico.h"

#include "acbrlib.h"
#include "application.h"
#include "doubledelegate.h"
#include "followup.h"
#include "noeditdelegate.h"
#include "produtoproxymodel.h"
#include "reaisdelegate.h"
#include "sqlquery.h"

#include <QSqlError>

WidgetCompraHistorico::WidgetCompraHistorico(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraHistorico) { ui->setupUi(this); }

WidgetCompraHistorico::~WidgetCompraHistorico() { delete ui; }

void WidgetCompraHistorico::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetCompraHistorico::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonAlteraCodForn, &QPushButton::clicked, this, &WidgetCompraHistorico::on_pushButtonAlteraCodForn_clicked, connectionType);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetCompraHistorico::on_pushButtonDanfe_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraHistorico::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->tablePedidos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetCompraHistorico::on_tablePedidos_selectionChanged, connectionType);
  connect(ui->treeView, &TreeView::doubleClicked, this, &WidgetCompraHistorico::on_treeView_doubleClicked, connectionType);
}

void WidgetCompraHistorico::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelCompras.select();
}

void WidgetCompraHistorico::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetCompraHistorico::setupTables() {
  modelCompras.setTable("view_compras_financeiro");

  ui->tablePedidos->setModel(&modelCompras);

  ui->tablePedidos->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->tablePedidos->hideColumn("Compra");

  //------------------------------------------------------

  modelProdutos.setTable("pedido_fornecedor_has_produto");

  //------------------------------------------------------

  modelProdutos2.setTable("pedido_fornecedor_has_produto2");

  //------------------------------------------------------

  // TODO: substituir view por query
  modelNFe.setTable("view_ordemcompra_nfe");

  modelNFe.setHeaderData("numeroNFe", "NF-e");

  ui->tableNFe->setModel(&modelNFe);

  ui->tableNFe->hideColumn("ordemCompra");
  ui->tableNFe->hideColumn("idNFe");

  //------------------------------------------------------

  modelFinanceiro.setTable("conta_a_pagar_has_pagamento");

  modelFinanceiro.setSort("dataPagamento");

  modelFinanceiro.setHeaderData("tipo", "Tipo");
  modelFinanceiro.setHeaderData("parcela", "Parcela");
  modelFinanceiro.setHeaderData("valor", "R$");
  modelFinanceiro.setHeaderData("dataPagamento", "Data");
  modelFinanceiro.setHeaderData("observacao", "Obs.");
  modelFinanceiro.setHeaderData("status", "Status");

  ui->tableFinanceiro->setModel(&modelFinanceiro);

  ui->tableFinanceiro->hideColumn("idPagamento");
  ui->tableFinanceiro->hideColumn("dataEmissao");
  ui->tableFinanceiro->hideColumn("idCompra");
  ui->tableFinanceiro->hideColumn("idVenda");
  ui->tableFinanceiro->hideColumn("idLoja");
  ui->tableFinanceiro->hideColumn("contraParte");
  ui->tableFinanceiro->hideColumn("idNFe");
  ui->tableFinanceiro->hideColumn("idCnab");
  ui->tableFinanceiro->hideColumn("nfe");
  ui->tableFinanceiro->hideColumn("dataRealizado");
  ui->tableFinanceiro->hideColumn("valorReal");
  ui->tableFinanceiro->hideColumn("tipoReal");
  ui->tableFinanceiro->hideColumn("parcelaReal");
  ui->tableFinanceiro->hideColumn("idConta");
  ui->tableFinanceiro->hideColumn("tipoDet");
  ui->tableFinanceiro->hideColumn("centroCusto");
  ui->tableFinanceiro->hideColumn("grupo");
  ui->tableFinanceiro->hideColumn("subGrupo");
  ui->tableFinanceiro->hideColumn("compraAvulsa");
  ui->tableFinanceiro->hideColumn("desativado");

  ui->tableFinanceiro->setItemDelegate(new NoEditDelegate(this));
}

void WidgetCompraHistorico::setTreeView() {
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
  modelTree.setHeaderData("desconto", "Desc. %");
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

  ui->treeView->hideColumn("idFollowup");
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

void WidgetCompraHistorico::on_tablePedidos_selectionChanged() {
  const auto selection = ui->tablePedidos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return; }

  const QString ordemCompra = modelCompras.data(selection.first().row(), "OC").toString();

  modelProdutos.setFilter("ordemCompra = " + ordemCompra);

  modelProdutos.select();

  modelProdutos2.setFilter("ordemCompra = " + ordemCompra);

  modelProdutos2.select();

  setTreeView();

  modelNFe.setFilter("ordemCompra = " + ordemCompra);

  modelNFe.select();

  const QString idCompra = modelCompras.data(selection.first().row(), "Compra").toString();

  modelFinanceiro.setFilter("idCompra IN (" + idCompra + ")");

  modelFinanceiro.select();
}

void WidgetCompraHistorico::on_lineEditBusca_textChanged() { montaFiltro(); }

void WidgetCompraHistorico::montaFiltro() {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = text.isEmpty() ? "0" : "(OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%')";

  modelCompras.setFilter(filtroBusca);
}

void WidgetCompraHistorico::on_pushButtonDanfe_clicked() {
  const auto selection = ui->tableNFe->selectionModel()->selectedRows();
  const auto rowCount = ui->tableNFe->rowCount();

  if (selection.isEmpty() and rowCount != 1) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const int row = (selection.isEmpty()) ? 0 : selection.first().row();
  const int idNFe = modelNFe.data(row, "idNFe").toInt();

  ACBrLib::gerarDanfe(idNFe);
}

void WidgetCompraHistorico::on_pushButtonFollowup_clicked() {
  const auto selection = ui->tablePedidos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const QString ordemCompra = modelCompras.data(selection.first().row(), "OC").toString();

  auto *followup = new FollowUp(ordemCompra, FollowUp::Tipo::Compra, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetCompraHistorico::on_treeView_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelTree.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelTree.data(modelTree.index(index.row(), modelTree.fieldIndex("idVenda")))); }
}

void WidgetCompraHistorico::on_pushButtonAlteraCodForn_clicked() {
  const auto selection = ui->treeView->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  QStringList ids;

  for (const auto &index : selection) {
    if (not index.parent().isValid()) { throw RuntimeError("Deve selecionar apenas sublinhas!", this); }

    const auto index2 = modelTree.proxyModel->mapToSource(index);
    const auto row = modelTree.mappedRow(index2);

    ids << modelProdutos2.data(row, "idPedido2").toString();
  }

  const QString text = QInputDialog::getText(this, "Cód Forn.", "Digite o novo código forn.:");

  if (text.isEmpty()) { return; }

  SqlQuery query;

  if (not query.exec("UPDATE pedido_fornecedor_has_produto2 SET codFornecedor = '" + text + "' WHERE idPedido2 IN (" + ids.join(", ") + ")")) {
    throw RuntimeException("Erro atualizando cód.: " + query.lastError().text());
  }

  modelProdutos2.select();
  qApp->enqueueInformation("Cód. forn. salvo com sucesso!");
}

// TODO: 1quando recalcula fluxo deve ter um campo para digitar/calcular ST pois o antigo é substituido e não é criado um novo
// TODO: 4verificar se dá para refazer/ajustar o fluxo de pagamento de acordo com as duplicatas da nota
// TODO: renomear essa classe para WidgetCompraHistorico
// TODO: duplo clique na coluna de idVenda deve abrir a Venda para facilitar
