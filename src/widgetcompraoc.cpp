#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "ui_widgetcompraoc.h"
#include "widgetcompraoc.h"

WidgetCompraOC::WidgetCompraOC(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraOC) { ui->setupUi(this); }

WidgetCompraOC::~WidgetCompraOC() { delete ui; }

void WidgetCompraOC::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelPedido.select()) { return; }
  if (not modelProduto.select()) { return; }
  if (not modelNFe.select()) { return; }
}

void WidgetCompraOC::resetTables() { modelIsSet = false; }

void WidgetCompraOC::setupTables() {
  modelPedido.setTable("view_ordemcompra_resumo");
  modelPedido.setEditStrategy(QSqlTableModel::OnManualSubmit);

  ui->tablePedido->setModel(&modelPedido);

  //------------------------------------------------------

  modelProduto.setTable("view_ordemcompra");
  modelProduto.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelProduto.setFilter("0");

  modelProduto.setHeaderData("statusPF", "Status Compra");
  modelProduto.setHeaderData("statusVP", "Status Venda");
  modelProduto.setHeaderData("idVenda", "Venda");
  modelProduto.setHeaderData("idVendaProdutoEHC", "Consumo");
  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("descricao", "Produto");
  modelProduto.setHeaderData("colecao", "Coleção");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("quant", "Quant.");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("un2", "Un2.");
  modelProduto.setHeaderData("caixas", "Cx.");
  modelProduto.setHeaderData("prcUnitario", "R$ Unit.");
  modelProduto.setHeaderData("preco", "R$");
  modelProduto.setHeaderData("kgcx", "Kg./Cx.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("codBarras", "Cód. Barras");
  modelProduto.setHeaderData("obs", "Obs.");

  ui->tableProduto->setModel(&modelProduto);
  ui->tableProduto->setItemDelegateForColumn("quant", new DoubleDelegate(this));
  ui->tableProduto->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProduto->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->tableProduto->setItemDelegateForColumn("kgcx", new DoubleDelegate(this));
  ui->tableProduto->hideColumn("idVendaProdutoPF");
  ui->tableProduto->hideColumn("idPedido");
  ui->tableProduto->hideColumn("idCompra");
  ui->tableProduto->hideColumn("ordemCompra");

  //------------------------------------------------------

  modelNFe.setTable("view_ordemcompra_nfe");
  modelNFe.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelNFe.setFilter("0");

  modelNFe.setHeaderData("numeroNFe", "NFe");

  ui->tableNFe->setModel(&modelNFe);
  ui->tableNFe->hideColumn("ordemCompra");
  ui->tableNFe->hideColumn("idNFe");
}

void WidgetCompraOC::setConnections() {
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraOC::on_lineEditBusca_textChanged);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetCompraOC::on_pushButtonDanfe_clicked);
  connect(ui->pushButtonDesfazerConsumo, &QPushButton::clicked, this, &WidgetCompraOC::on_pushButtonDesfazerConsumo_clicked);
  connect(ui->tablePedido, &TableView::clicked, this, &WidgetCompraOC::on_tablePedido_clicked);
}

void WidgetCompraOC::on_tablePedido_clicked(const QModelIndex &index) {
  const QString oc = modelPedido.data(index.row(), "OC").toString();

  modelProduto.setFilter("ordemCompra = " + oc);

  if (not modelProduto.select()) { return; }

  modelNFe.setFilter("ordemCompra = " + oc);

  if (not modelNFe.select()) { return; }
}

void WidgetCompraOC::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableNFe->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  if (ACBr acbr; not acbr.gerarDanfe(modelNFe.data(list.first().row(), "idNFe").toInt())) { return; }
}

void WidgetCompraOC::on_pushButtonDesfazerConsumo_clicked() { // TODO: this should update produto_estoque.quant
  const auto list = ui->tableProduto->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  const int row = list.first().row();

  const int idVendaConsumo = modelProduto.data(row, "idVendaProdutoEHC").toInt();

  if (idVendaConsumo == 0) { return qApp->enqueueError("A linha não possui consumo associado!"); }

  const QString status = modelProduto.data(row, "statusVP").toString();

  if (status == "ENTREGA AGEND." or status == "EM ENTREGA" or status == "ENTREGUE") { return qApp->enqueueError("Produto está em entrega/entregue!"); }

  QMessageBox msgBox(QMessageBox::Question, "Desfazer consumo?", "Tem certeza?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Continuar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  const QString idVenda = modelProduto.data(row, "idVenda").toString();
  const int idVendaProduto = modelProduto.data(row, "idVendaProdutoPF").toInt();

  if (not qApp->startTransaction()) { return; }

  if (not desfazerConsumo(idVendaProduto, idVendaConsumo)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();

  qApp->enqueueInformation("Operação realizada com sucesso!");
}

bool WidgetCompraOC::desfazerConsumo(const int idVendaProduto, const int idVendaConsumo) {
  // REFAC: pass this responsability to Estoque class?

  // NOTE: estoque_has_consumo may have the same idVendaProduto in more than one row
  if (idVendaConsumo != 0) {
    QSqlQuery queryDelete;
    queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idVendaProduto = :idVendaProduto");
    // TODO: change this to idPedido?
    queryDelete.bindValue(":idVendaProduto", idVendaConsumo);

    if (not queryDelete.exec()) { return qApp->enqueueError(false, "Erro removendo consumo estoque: " + queryDelete.lastError().text()); }
  }

  // TODO: juntar linhas sem consumo do mesmo tipo? (usar idRelacionado)

  QSqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto SET idVenda = NULL, idVendaProduto = NULL WHERE idVendaProduto = :idVendaProduto AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");
  queryCompra.bindValue(":idVendaProduto", idVendaProduto);

  if (not queryCompra.exec()) { return qApp->enqueueError(false, "Erro atualizando pedido compra: " + queryCompra.lastError().text()); }

  QSqlQuery queryVenda;
  queryVenda.prepare(
      "UPDATE venda_has_produto SET status = CASE WHEN reposicaoEntrega THEN 'REPO. ENTREGA' WHEN reposicaoReceb THEN 'REPO. RECEB.' ELSE 'PENDENTE' END, idCompra = NULL, dataPrevCompra = "
      "NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, dataPrevReceb = "
      "NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE idVendaProduto = :idVendaProduto AND status NOT IN ('CANCELADO', 'DEVOLVIDO')");
  queryVenda.bindValue(":idVendaProduto", idVendaProduto);

  if (not queryVenda.exec()) { return qApp->enqueueError(false, "Erro atualizando pedido venda: " + queryVenda.lastError().text()); }

  return true;
}

void WidgetCompraOC::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetCompraOC::montaFiltro() {
  const QString text = ui->lineEditBusca->text();

  modelPedido.setFilter("Venda LIKE '%" + text + "%' OR OC LIKE '%" + text + "%'");

  if (not modelPedido.select()) { return; }
}

// TODO: alterar filtros, muito ruim de achar coisas nessa tela
