#include <QMessageBox>
#include <QSqlError>

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "inputdialogfinanceiro.h"
#include "reaisdelegate.h"
#include "ui_widgethistoricocompra.h"
#include "widgethistoricocompra.h"

WidgetHistoricoCompra::WidgetHistoricoCompra(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetHistoricoCompra) { ui->setupUi(this); }

WidgetHistoricoCompra::~WidgetHistoricoCompra() { delete ui; }

void WidgetHistoricoCompra::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetHistoricoCompra::on_lineEditBusca_textChanged, connectionType);
  connect(ui->tablePedidos, &TableView::clicked, this, &WidgetHistoricoCompra::on_tablePedidos_clicked, connectionType);
  connect(ui->tableNFe, &TableView::activated, this, &WidgetHistoricoCompra::on_tableNFe_activated, connectionType);
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

  if (not modelViewComprasFinanceiro.select()) { return; }
}

void WidgetHistoricoCompra::resetTables() { modelIsSet = false; }

void WidgetHistoricoCompra::setupTables() {
  modelViewComprasFinanceiro.setTable("view_compras_financeiro");

  ui->tablePedidos->setModel(&modelViewComprasFinanceiro);

  ui->tablePedidos->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->tablePedidos->hideColumn("Compra");

  //------------------------------------------------------

  modelProdutos.setTable("pedido_fornecedor_has_produto");

  modelProdutos.setHeaderData("status", "Status");
  modelProdutos.setHeaderData("statusFinanceiro", "Financeiro");
  modelProdutos.setHeaderData("ordemRepresentacao", "OC Rep");
  modelProdutos.setHeaderData("idVenda", "Venda");
  modelProdutos.setHeaderData("descricao", "Produto");
  modelProdutos.setHeaderData("colecao", "Coleção");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("un2", "Un2.");
  modelProdutos.setHeaderData("caixas", "Cx.");
  modelProdutos.setHeaderData("prcUnitario", "R$ Unit.");
  modelProdutos.setHeaderData("preco", "R$");
  modelProdutos.setHeaderData("kgcx", "Kg./Cx.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");
  modelProdutos.setHeaderData("codBarras", "Cód. Barras");
  modelProdutos.setHeaderData("obs", "Obs.");
  modelProdutos.setHeaderData("dataPrevCompra", "Prev. Compra");
  modelProdutos.setHeaderData("dataRealCompra", "Compra");
  modelProdutos.setHeaderData("dataPrevConf", "Prev. Confirmação");
  modelProdutos.setHeaderData("dataRealConf", "Confirmação");
  modelProdutos.setHeaderData("dataPrevFat", "Prev. Faturamento");
  modelProdutos.setHeaderData("dataRealFat", "Faturamento");
  modelProdutos.setHeaderData("dataPrevColeta", "Prev. Coleta");
  modelProdutos.setHeaderData("dataRealColeta", "Coleta");
  modelProdutos.setHeaderData("dataPrevReceb", "Prev. Recebimento");
  modelProdutos.setHeaderData("dataRealReceb", "Recebimento");
  modelProdutos.setHeaderData("dataPrevEnt", "Prev. Entrega");
  modelProdutos.setHeaderData("dataRealEnt", "Entrega");

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("kgcx", new DoubleDelegate(this));

  ui->tableProdutos->hideColumn("idRelacionado");
  ui->tableProdutos->hideColumn("idPedido1");
  ui->tableProdutos->hideColumn("selecionado");
  ui->tableProdutos->hideColumn("aliquotaSt");
  ui->tableProdutos->hideColumn("st");
  ui->tableProdutos->hideColumn("ordemCompra");
  ui->tableProdutos->hideColumn("idVendaProduto1");
  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("idCompra");
  ui->tableProdutos->hideColumn("fornecedor");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("quantUpd");
  ui->tableProdutos->hideColumn("quantConsumida");

  //------------------------------------------------------

  modelNFe.setTable("view_ordemcompra_nfe");

  modelNFe.setHeaderData("numeroNFe", "NFe");

  ui->tableNFe->setModel(&modelNFe);

  ui->tableNFe->hideColumn("idCompra");
  ui->tableNFe->hideColumn("ordemCompra");
  ui->tableNFe->hideColumn("idNFe");
}

void WidgetHistoricoCompra::on_tablePedidos_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString idCompra = modelViewComprasFinanceiro.data(index.row(), "Compra").toString();

  modelProdutos.setFilter("idCompra = " + idCompra);

  if (not modelProdutos.select()) { return qApp->enqueueError("Erro buscando produtos: " + modelProdutos.lastError().text(), this); }

  modelNFe.setFilter("idCompra = " + idCompra);

  if (not modelNFe.select()) { return qApp->enqueueError("Erro buscando NFe: " + modelNFe.lastError().text(), this); }
}

void WidgetHistoricoCompra::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetHistoricoCompra::montaFiltro() {
  const QString text = ui->lineEditBusca->text();
  const QString filtroBusca = text.isEmpty() ? "" : "OC LIKE '%" + text + "%' OR Código LIKE '%" + text + "%'";

  modelViewComprasFinanceiro.setFilter(filtroBusca);
}

void WidgetHistoricoCompra::on_tableNFe_activated(const QModelIndex &index) {
  if (ACBr acbr; not acbr.gerarDanfe(modelNFe.data(index.row(), "idNFe").toInt())) { return; }
}

// TODO: 1quando recalcula fluxo deve ter um campo para digitar/calcular ST pois o antigo é substituido e não é criado um novo
// TODO: 4associar notas com cada produto? e verificar se dá para refazer/ajustar o fluxo de pagamento de acordo com as duplicatas da nota

// TODO: converter tabela inferior para arvore
