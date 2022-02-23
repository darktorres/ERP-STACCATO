#include "widgetcomprafaturar.h"
#include "ui_widgetcomprafaturar.h"

#include "application.h"
#include "cancelaproduto.h"
#include "followup.h"
#include "importarxml.h"
#include "inputdialog.h"
#include "inputdialogproduto.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

WidgetCompraFaturar::WidgetCompraFaturar(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraFaturar) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetCompraFaturar::~WidgetCompraFaturar() { delete ui; }

void WidgetCompraFaturar::setupTables() {
  modelResumo.setTable("view_fornecedor_compra_faturar");

  modelResumo.setFilter("");

  ui->tableResumo->setModel(&modelResumo);

  ui->tableResumo->hideColumn("representacao");

  //---------------------------------------------------------------------------

  modelFaturamento.setTable("view_faturamento");

  modelFaturamento.setSort("ordemCompra");

  modelFaturamento.setHeaderData("ordemCompra", "O.C.");
  modelFaturamento.setHeaderData("fornecedor", "Fornecedor");
  modelFaturamento.setHeaderData("data", "Data Venda");
  modelFaturamento.setHeaderData("produtos", "Produtos");
  modelFaturamento.setHeaderData("total", "Total");
  modelFaturamento.setHeaderData("dataPrevFat", "Prev. Fat.");
  modelFaturamento.setHeaderData("vendedor", "Vendedor");
  modelFaturamento.setHeaderData("idVenda", "Venda");
  modelFaturamento.setHeaderData("dataFollowup", "Data Followup");
  modelFaturamento.setHeaderData("observacao", "Observação");

  ui->table->setModel(&modelFaturamento);

  ui->table->setItemDelegateForColumn("total", new ReaisDelegate(this));

  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("representacao");
}

void WidgetCompraFaturar::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &WidgetCompraFaturar::on_checkBoxRepresentacao_toggled, connectionType);
  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonLimparFiltro, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonLimparFiltro_clicked, connectionType);
  connect(ui->pushButtonMarcarFaturado, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonReagendar_clicked, connectionType);
  connect(ui->table, &TableView::doubleClicked, this, &WidgetCompraFaturar::on_table_doubleClicked, connectionType);
  connect(ui->tableResumo, &QTableView::clicked, this, &WidgetCompraFaturar::on_tableResumo_clicked, connectionType);
}

void WidgetCompraFaturar::updateTables() {
  if (not isSet) {
    setupTables();
    montaFiltro();
    setConnections();
    isSet = true;
  }

  modelResumo.select();

  //--------------------------------------

  modelFaturamento.select();
}

void WidgetCompraFaturar::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetCompraFaturar::faturarRepresentacao(const QDate dataReal, const QStringList &idsCompra) {
  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM ENTREGA', dataRealFat = :dataRealFat WHERE status = 'EM FATURAMENTO' AND idCompra = :idCompra");

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'EM ENTREGA' WHERE status = 'EM FATURAMENTO' AND idCompra = :idCompra");

  for (const auto &idCompra : idsCompra) {
    queryCompra.bindValue(":dataRealFat", dataReal);
    queryCompra.bindValue(":idCompra", idCompra);

    if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando status da compra: " + queryCompra.lastError().text()); }

    queryVenda.bindValue(":idCompra", idCompra);

    if (not queryVenda.exec()) { throw RuntimeException("Erro atualizando status do produto da venda: " + queryVenda.lastError().text()); }
  }
}

void WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Não selecionou nenhuma compra!", this); }

  QStringList idsCompra;
  QStringList fornecedores;
  QStringList idVendas;

  for (const auto &index : selection) {
    idsCompra << modelFaturamento.data(index.row(), "idCompra").toString();
    fornecedores << modelFaturamento.data(index.row(), "fornecedor").toString();
    idVendas << modelFaturamento.data(index.row(), "idVenda").toString();
  }

  const int size = fornecedores.size();

  if (fornecedores.removeDuplicates() != size - 1) { throw RuntimeError("Fornecedores diferentes!", this); }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::Faturamento, this);
  inputDlg.setFilter(idsCompra);

  if (inputDlg.exec() != InputDialogProduto::Accepted) { return; }

  const QDate dataFaturamento = inputDlg.getDate();

  SqlQuery query;

  if (not query.exec("SELECT representacao FROM fornecedor WHERE razaoSocial = '" + fornecedores.first() + "'")) {
    throw RuntimeException("Erro verificando se fornecedor é representação: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Fornecedor não encontrado!"); }

  const bool isRepresentacao = query.value("representacao").toBool();

  const bool pularNota = (isRepresentacao or fornecedores.first() == "ATELIER STACCATO");

  if (pularNota) {
    qApp->startTransaction("WidgetCompraFaturar::on_pushButtonMarcarFaturado_pularNota");
    faturarRepresentacao(dataFaturamento, idsCompra);
    qApp->endTransaction();
  } else {
    auto *import = new ImportarXML(idsCompra, dataFaturamento, this);
    import->setAttribute(Qt::WA_DeleteOnClose);
    import->showMaximized();

    if (import->exec() != QDialog::Accepted) { return; }
  }

  qApp->startTransaction("WidgetCompraFaturar::on_pushButtonMarcarFaturado");
  Sql::updateVendaStatus(idVendas);
  qApp->endTransaction();

  updateTables();
  qApp->enqueueInformation("Confirmado faturamento!", this);
}

void WidgetCompraFaturar::montaFiltro() {
  const bool representacao = ui->checkBoxRepresentacao->isChecked();

  modelFaturamento.setFilter("representacao = " + QString(representacao ? "TRUE" : "FALSE"));
}

void WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  if (selection.size() > 1) { throw RuntimeError("Selecione apenas uma linha!", this); }

  auto *cancelaProduto = new CancelaProduto(CancelaProduto::Tipo::CompraFaturamento, this);
  cancelaProduto->setAttribute(Qt::WA_DeleteOnClose);
  cancelaProduto->setFilter(modelFaturamento.data(selection.first().row(), "ordemCompra").toString());
}

void WidgetCompraFaturar::on_pushButtonReagendar_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::ReagendarFaturamento, this);
  if (input.exec() != InputDialog::Accepted) { return; }

  const QDate dataPrevista = input.getNextDate();

  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");

  for (const auto &index : selection) {
    const int idCompra = modelFaturamento.data(index.row(), "idCompra").toInt();

    queryCompra.bindValue(":dataPrevFat", dataPrevista);
    queryCompra.bindValue(":idCompra", idCompra);

    if (not queryCompra.exec()) { throw RuntimeException("Erro query pedido_fornecedor: " + queryCompra.lastError().text(), this); }

    queryVenda.bindValue(":dataPrevFat", dataPrevista);
    queryVenda.bindValue(":idCompra", idCompra);

    if (not queryVenda.exec()) { throw RuntimeException("Erro query venda_has_produto2: " + queryVenda.lastError().text(), this); }
  }

  updateTables();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void WidgetCompraFaturar::on_checkBoxRepresentacao_toggled(const bool checked) {
  ui->pushButtonMarcarFaturado->setText(checked ? "Marcar faturado" : "Marcar faturado - Importar NF-e");
  montaFiltro();
}

void WidgetCompraFaturar::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const QString ordemCompra = modelFaturamento.data(selection.first().row(), "ordemCompra").toString();

  auto *followup = new FollowUp(ordemCompra, FollowUp::Tipo::Compra, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetCompraFaturar::on_tableResumo_clicked(const QModelIndex &index) {
  if (not index.isValid()) {
    modelFaturamento.setFilter("");
    ui->checkBoxRepresentacao->setChecked(false);
    return;
  }

  const QString fornecedor = modelResumo.data(index.row(), "fornecedor").toString();

  const QString filtro = "fornecedor = '" + fornecedor + "'";

  modelFaturamento.setFilter(filtro);

  // --------------------------------------------

  const bool isRepresentacao = modelResumo.data(index.row(), "representacao").toBool();

  ui->checkBoxRepresentacao->setChecked(isRepresentacao);
}

void WidgetCompraFaturar::on_pushButtonLimparFiltro_clicked() {
  ui->tableResumo->clearSelection();

  const QString fornecedor = "";

  const QString filtro = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";

  modelFaturamento.setFilter(filtro);
}

void WidgetCompraFaturar::on_table_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelFaturamento.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelFaturamento.data(index.row(), "idVenda")); }
}

// TODO: 4quando importar nota vincular com as contas_pagar
