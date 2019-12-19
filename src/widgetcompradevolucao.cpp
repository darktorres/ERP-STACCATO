#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "sql.h"
#include "ui_widgetcompradevolucao.h"
#include "widgetcompradevolucao.h"

WidgetCompraDevolucao::WidgetCompraDevolucao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraDevolucao) { ui->setupUi(this); }

WidgetCompraDevolucao::~WidgetCompraDevolucao() { delete ui; }

void WidgetCompraDevolucao::resetTables() { modelIsSet = false; }

void WidgetCompraDevolucao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonDevolucaoFornecedor, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked, connectionType);
  connect(ui->pushButtonRetornarEstoque, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked, connectionType);
  connect(ui->radioButtonFiltroPendente, &QRadioButton::clicked, this, &WidgetCompraDevolucao::on_radioButtonFiltroPendente_clicked, connectionType);
  connect(ui->radioButtonFiltroDevolvido, &QRadioButton::clicked, this, &WidgetCompraDevolucao::on_radioButtonFiltroDevolvido_clicked, connectionType);
}

void WidgetCompraDevolucao::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelVendaProduto.select()) { return; }
}

void WidgetCompraDevolucao::setupTables() {
  modelVendaProduto.setTable("venda_has_produto2");

  modelVendaProduto.setSort("idVenda");

  modelVendaProduto.setHeaderData("status", "Status");
  modelVendaProduto.setHeaderData("statusOriginal", "Status Original");
  modelVendaProduto.setHeaderData("fornecedor", "Fornecedor");
  modelVendaProduto.setHeaderData("idVenda", "Venda");
  modelVendaProduto.setHeaderData("produto", "Produto");
  modelVendaProduto.setHeaderData("obs", "Obs.");
  modelVendaProduto.setHeaderData("lote", "Lote");
  modelVendaProduto.setHeaderData("caixas", "Cx.");
  modelVendaProduto.setHeaderData("quant", "Quant.");
  modelVendaProduto.setHeaderData("un", "Un.");
  modelVendaProduto.setHeaderData("unCaixa", "Un./Cx.");
  modelVendaProduto.setHeaderData("codComercial", "Cód. Com.");
  modelVendaProduto.setHeaderData("formComercial", "Form. Com.");

  ui->table->setModel(&modelVendaProduto);

  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("idVendaProdutoFK");
  ui->table->hideColumn("idRelacionado");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("entregou");
  ui->table->hideColumn("recebeu");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("idNFeSaida");
  ui->table->hideColumn("idNFeFutura");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("prcUnitario");
  ui->table->hideColumn("descUnitario");
  ui->table->hideColumn("parcial");
  ui->table->hideColumn("desconto");
  ui->table->hideColumn("parcialDesc");
  ui->table->hideColumn("descGlobal");
  ui->table->hideColumn("total");
  ui->table->hideColumn("mostrarDesconto");
  ui->table->hideColumn("estoque");
  ui->table->hideColumn("promocao");
  ui->table->hideColumn("reposicaoEntrega");
  ui->table->hideColumn("reposicaoReceb");
  ui->table->hideColumn("dataPrevCompra");
  ui->table->hideColumn("dataRealCompra");
  ui->table->hideColumn("dataPrevConf");
  ui->table->hideColumn("dataRealConf");
  ui->table->hideColumn("dataPrevFat");
  ui->table->hideColumn("dataRealFat");
  ui->table->hideColumn("dataPrevColeta");
  ui->table->hideColumn("dataRealColeta");
  ui->table->hideColumn("dataPrevReceb");
  ui->table->hideColumn("dataRealReceb");
  ui->table->hideColumn("dataPrevEnt");
  ui->table->hideColumn("dataRealEnt");
}

void WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked() {
  // NOTE: colocar produto no fluxo da logistica para devolver ao fornecedor?
  // TODO: 2criar nota de devolucao caso tenha recebido nota (troca cfop, inverte remetente/destinario, nat. 'devolucao
  // de mercadoria'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Não selecionou nenhuma linha!", this); }

  const QString idVenda = modelVendaProduto.data(list.first().row(), "idVenda").toString();

  if (not qApp->startTransaction()) { return; }

  if (not retornarFornecedor(list)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  if (not modelVendaProduto.select()) { return; }

  qApp->enqueueInformation("Retornado para fornecedor!", this);
}

bool WidgetCompraDevolucao::retornarEstoque(const QModelIndexList &list) {
  // TODO: ao fazer a linha copia da devolucao limpar todas as datas e preencher 'dataRealEnt' com a data em que foi feita o retorno para o estoque
  // TODO: add quant too?
  // TODO: e se tiver varios consumos?

  for (const auto &index : list) {
    const QString status = modelVendaProduto.data(index.row(), "statusOriginal").toString();

    if (not modelVendaProduto.setData(index.row(), "status", "DEVOLVIDO ESTOQUE")) { return false; }

    QSqlQuery query;
    query.prepare("DELETE FROM estoque_has_consumo WHERE idVendaProduto2 = :idVendaProduto2");
    query.bindValue(":idVendaProduto2", modelVendaProduto.data(index.row(), "idVendaProduto2"));

    if (not query.exec()) { return qApp->enqueueError(false, "Erro ajustando consumo: " + query.lastError().text(), this); }
  }

  return modelVendaProduto.submitAll();
}

bool WidgetCompraDevolucao::retornarFornecedor(const QModelIndexList &list) {
  for (const auto &index : list) {
    if (not modelVendaProduto.setData(index.row(), "status", "DEVOLVIDO FORN.")) { return false; }
  }

  if (not modelVendaProduto.submitAll()) { return false; }

  return true;
}

void WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Não selecionou nenhuma linha!", this); }

  const QString idVenda = modelVendaProduto.data(list.first().row(), "idVenda").toString();

  if (not qApp->startTransaction()) { return; }

  if (not retornarEstoque(list)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVenda)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  if (not modelVendaProduto.select()) { return; }

  qApp->enqueueInformation("Retornado para estoque!", this);
}

void WidgetCompraDevolucao::on_radioButtonFiltroPendente_clicked(const bool) { montaFiltro(); }

void WidgetCompraDevolucao::on_radioButtonFiltroDevolvido_clicked(const bool) { montaFiltro(); }

void WidgetCompraDevolucao::montaFiltro() {
  const bool isPendente = ui->radioButtonFiltroPendente->isChecked();

  ui->pushButtonDevolucaoFornecedor->setEnabled(isPendente);
  ui->pushButtonRetornarEstoque->setEnabled(isPendente);

  modelVendaProduto.setFilter("quant < 0 AND " + QString(isPendente ? "status = 'PENDENTE DEV.'" : "status != 'PENDENTE DEV.'"));
}
