#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "cancelaproduto.h"
#include "importarxml.h"
#include "inputdialog.h"
#include "inputdialogproduto.h"
#include "reaisdelegate.h"
#include "ui_widgetcomprafaturar.h"
#include "widgetcomprafaturar.h"

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
  connect(ui->pushButtonMarcarFaturado, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonReagendar_clicked, connectionType);
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

  if (not modelResumo.select()) { return; }

  //--------------------------------------

  if (not modelViewFaturamento.select()) { return; }
}

void WidgetCompraFaturar::resetTables() { modelIsSet = false; }

bool WidgetCompraFaturar::faturarRepresentacao(const QDate &dataReal, const QStringList &idsCompra) {
  QSqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM ENTREGA', dataRealFat = :dataRealFat WHERE status = 'EM FATURAMENTO' AND idCompra = :idCompra");

  QSqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'EM ENTREGA' WHERE status = 'EM FATURAMENTO' AND idCompra = :idCompra");

  for (const auto &idCompra : idsCompra) {
    queryCompra.bindValue(":dataRealFat", dataReal);
    queryCompra.bindValue(":idCompra", idCompra);

    if (not queryCompra.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + queryCompra.lastError().text(), this); }

    queryVenda.bindValue(":idCompra", idCompra);

    if (not queryVenda.exec()) { return qApp->enqueueError(false, "Erro atualizando status do produto da venda: " + queryVenda.lastError().text(), this); }
  }

  return true;
}

void WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Não selecionou nenhuma compra!", this); }

  QStringList idsCompra;
  QStringList fornecedores;

  for (const auto &index : list) {
    idsCompra << modelViewFaturamento.data(index.row(), "idCompra").toString();
    fornecedores << modelViewFaturamento.data(index.row(), "fornecedor").toString();
  }

  const int size = fornecedores.size();

  if (fornecedores.removeDuplicates() != size - 1) { return qApp->enqueueError("Fornecedores diferentes!", this); }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::Faturamento, this);
  if (not inputDlg.setFilter(idsCompra)) { return; }
  if (inputDlg.exec() != InputDialogProduto::Accepted) { return; }

  const QDate dataReal = inputDlg.getDate();

  const bool pularNota = ui->checkBoxRepresentacao->isChecked() or fornecedores.first() == "ATELIER";

  if (pularNota) {
    if (not qApp->startTransaction()) { return; }
    if (not faturarRepresentacao(dataReal, idsCompra)) { return qApp->rollbackTransaction(); }
    if (not qApp->endTransaction()) { return; }
  } else {
    auto *import = new ImportarXML(idsCompra, dataReal, this);
    import->setAttribute(Qt::WA_DeleteOnClose);
    import->showMaximized();

    if (import->exec() != QDialog::Accepted) { return; }
  }

  updateTables();
  qApp->enqueueInformation("Confirmado faturamento!", this);
}

void WidgetCompraFaturar::montaFiltro() {
  const bool representacao = ui->checkBoxRepresentacao->isChecked();

  modelViewFaturamento.setFilter("representacao = " + QString(representacao ? "TRUE" : "FALSE"));
}

void WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  if (list.size() > 1) { return qApp->enqueueError("Selecione apenas uma linha!", this); }

  auto cancelaProduto = new CancelaProduto(CancelaProduto::Tipo::CompraFaturamento, this);
  cancelaProduto->setAttribute(Qt::WA_DeleteOnClose);
  cancelaProduto->setFilter(modelViewFaturamento.data(list.first().row(), "OC").toString());
}

void WidgetCompraFaturar::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::Faturamento, this);
  if (input.exec() != InputDialog::Accepted) { return; }

  const QDate dataPrevista = input.getNextDate();

  QSqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");

  QSqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");

  for (const auto &index : list) {
    const int idCompra = modelViewFaturamento.data(index.row(), "idCompra").toInt();

    queryCompra.bindValue(":dataPrevFat", dataPrevista);
    queryCompra.bindValue(":idCompra", idCompra);

    if (not queryCompra.exec()) { return qApp->enqueueError("Erro query pedido_fornecedor: " + queryCompra.lastError().text(), this); }

    queryVenda.bindValue(":dataPrevFat", dataPrevista);
    queryVenda.bindValue(":idCompra", idCompra);

    if (not queryVenda.exec()) { return qApp->enqueueError("Erro query venda_has_produto2: " + queryVenda.lastError().text(), this); }
  }

  updateTables();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

// TODO: 4quando importar nota vincular com as contas_pagar
// TODO: 5reimportar nota id 4936 que veio com o produto dividido para testar o quantConsumido
// TODO: 5reestruturar na medida do possivel de forma que cada estoque tenha apenas uma nota/compra
// TODO: 0colocar tela de busca

void WidgetCompraFaturar::on_checkBoxRepresentacao_toggled(bool checked) {
  ui->pushButtonMarcarFaturado->setText(checked ? "Marcar faturado" : "Marcar faturado - Importar NFe");
  montaFiltro();
}
