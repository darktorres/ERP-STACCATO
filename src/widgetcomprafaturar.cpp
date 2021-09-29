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

#include <QDate>
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

  //---------------------------------------------------------------------------

  modelViewFaturamento.setTable("view_faturamento");

  modelViewFaturamento.setSort("OC");

  modelViewFaturamento.setHeaderData("dataPrevFat", "Prev. Fat.");

  ui->table->setModel(&modelViewFaturamento);

  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));

  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("representacao");
}

void WidgetCompraFaturar::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &WidgetCompraFaturar::on_checkBoxRepresentacao_toggled, connectionType);
  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonMarcarFaturado, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonReagendar_clicked, connectionType);
  connect(ui->tableResumo, &QTableView::clicked, this, &WidgetCompraFaturar::on_tableResumo_clicked, connectionType);
}

void WidgetCompraFaturar::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelResumo.select();

  //--------------------------------------

  modelViewFaturamento.select();
}

void WidgetCompraFaturar::resetTables() { modelIsSet = false; }

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
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Não selecionou nenhuma compra!", this); }

  QStringList idsCompra;
  QStringList fornecedores;
  QStringList idVendas;

  for (const auto &index : list) {
    idsCompra << modelViewFaturamento.data(index.row(), "idCompra").toString();
    fornecedores << modelViewFaturamento.data(index.row(), "fornecedor").toString();
    idVendas << modelViewFaturamento.data(index.row(), "Código").toString();
  }

  const int size = fornecedores.size();

  if (fornecedores.removeDuplicates() != size - 1) { throw RuntimeError("Fornecedores diferentes!", this); }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::Faturamento, this);
  inputDlg.setFilter(idsCompra);

  if (inputDlg.exec() != InputDialogProduto::Accepted) { return; }

  const QDate dataFaturamento = inputDlg.getDate();

  const bool pularNota = (ui->checkBoxRepresentacao->isChecked() or fornecedores.first() == "ATELIER STACCATO");

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

  modelViewFaturamento.setFilter("representacao = " + QString(representacao ? "TRUE" : "FALSE"));
}

void WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  if (list.size() > 1) { throw RuntimeError("Selecione apenas uma linha!", this); }

  auto *cancelaProduto = new CancelaProduto(CancelaProduto::Tipo::CompraFaturamento, this);
  cancelaProduto->setAttribute(Qt::WA_DeleteOnClose);
  cancelaProduto->setFilter(modelViewFaturamento.data(list.first().row(), "OC").toString());
}

void WidgetCompraFaturar::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::Faturamento, this);
  if (input.exec() != InputDialog::Accepted) { return; }

  const QDate dataPrevista = input.getNextDate();

  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");

  for (const auto &index : list) {
    const int idCompra = modelViewFaturamento.data(index.row(), "idCompra").toInt();

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
  ui->pushButtonMarcarFaturado->setText(checked ? "Marcar faturado" : "Marcar faturado - Importar NFe");
  montaFiltro();
}

void WidgetCompraFaturar::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeException("Nenhuma linha selecionada!"); }

  const QString ordemCompra = modelViewFaturamento.data(selection.first().row(), "OC").toString();

  auto *followup = new FollowUp(ordemCompra, FollowUp::Tipo::Compra, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetCompraFaturar::on_tableResumo_clicked(const QModelIndex &index) {
  const QString fornecedor = index.isValid() ? modelResumo.data(index.row(), "fornecedor").toString() : "";

  const QString filtro = fornecedor.isEmpty() ? "" : "fornecedor = '" + fornecedor + "'";

  modelViewFaturamento.setFilter(filtro);
}

// TODO: 4quando importar nota vincular com as contas_pagar
