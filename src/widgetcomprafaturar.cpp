#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "importarxml.h"
#include "inputdialog.h"
#include "inputdialogproduto.h"
#include "reaisdelegate.h"
#include "sql.h"
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

  ui->tableResumo->setModel(&modelResumo);

  //---------------------------------------------------------------------------

  modelViewFaturamento.setTable("view_faturamento");

  modelViewFaturamento.setHeaderData("dataPrevFat", "Prev. Fat.");

  ui->table->setModel(&modelViewFaturamento);
  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("representacao");
}

void WidgetCompraFaturar::setConnections() {
  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &WidgetCompraFaturar::montaFiltro);
  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked);
  connect(ui->pushButtonMarcarFaturado, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetCompraFaturar::on_pushButtonReagendar_clicked);
  connect(ui->table, &TableView::entered, this, &WidgetCompraFaturar::on_table_entered);
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

  ui->tableResumo->resizeColumnsToContents();

  if (not modelViewFaturamento.select()) { return; }

  ui->table->resizeColumnsToContents();
}

void WidgetCompraFaturar::resetTables() { modelIsSet = false; }

bool WidgetCompraFaturar::faturarRepresentacao(const QDateTime &dataReal, const QStringList &idsCompra) {
  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM ENTREGA', dataRealFat = :dataRealFat WHERE idCompra = :idCompra");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET status = 'EM ENTREGA' WHERE idCompra = :idCompra AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  for (const auto &idCompra : idsCompra) {
    query1.bindValue(":dataRealFat", dataReal);
    query1.bindValue(":idCompra", idCompra);

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + query1.lastError().text()); }

    query2.bindValue(":idCompra", idCompra);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando status do produto da venda: " + query2.lastError().text()); }
  }

  return true;
}

void WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Não selecionou nenhuma compra!"); }

  QStringList idsCompra;
  QStringList fornecedores;
  QStringList idVendas;

  for (const auto &item : list) {
    idsCompra << modelViewFaturamento.data(item.row(), "idCompra").toString();
    fornecedores << modelViewFaturamento.data(item.row(), "fornecedor").toString();
    idVendas << modelViewFaturamento.data(item.row(), "Código").toString();
  }

  const int size = fornecedores.size();

  if (fornecedores.removeDuplicates() != size - 1) { return qApp->enqueueError("Fornecedores diferentes!"); }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::Faturamento);
  if (not inputDlg.setFilter(idsCompra)) { return; }
  if (inputDlg.exec() != InputDialogProduto::Accepted) { return; }

  const QDateTime dataReal = inputDlg.getDate();

  // TODO: 0quando a sigla CAMB pular

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

  // TODO: put this inside a transaction
  if (not Sql::updateVendaStatus(idVendas.join(", "))) { return; }

  updateTables();
  qApp->enqueueInformation("Confirmado faturamento!");
}

void WidgetCompraFaturar::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetCompraFaturar::montaFiltro() {
  const bool representacao = ui->checkBoxRepresentacao->isChecked();

  modelViewFaturamento.setFilter("representacao = " + QString(representacao ? "TRUE" : "FALSE"));

  if (not modelViewFaturamento.select()) { return; }

  ui->table->resizeColumnsToContents();
}

bool WidgetCompraFaturar::cancelar(const QModelIndexList &list) {
  // TODO: 0nas outras telas com cancelamento verificar se estou filtrando
  // TODO: 5verificar como tratar conta_a_pagar_has_pagamento
  // TODO: alterar a funcao de cancelar por uma tela de SAC onde o usuario indica as operacoes necessarias (troca de nfe, produto nao disponivel etc) e realiza as mudanças necessarias, bem como
  // alteracoes no fluxo de pagamento se necessario

  QSqlQuery query1;
  query1.prepare("UPDATE venda_has_produto SET status = 'PENDENTE', idCompra = NULL WHERE status = 'EM FATURAMENTO' AND idVendaProduto IN (SELECT idVendaProduto FROM pedido_fornecedor_has_produto "
                 "WHERE ordemCompra = :ordemCompra AND status = 'EM FATURAMENTO') AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'CANCELADO' WHERE ordemCompra = :ordemCompra AND status = 'EM FATURAMENTO'");

  for (const auto &item : list) {
    query1.bindValue(":ordemCompra", modelViewFaturamento.data(item.row(), "OC"));

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro buscando dados: " + query1.lastError().text()); }

    query2.bindValue(":ordemCompra", modelViewFaturamento.data(item.row(), "OC"));

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro salvando dados: " + query2.lastError().text()); }
  }

  return true;
}

void WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  QStringList idVendas;

  for (const auto &item : list) { idVendas << modelViewFaturamento.data(item.row(), "Código").toString(); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Essa operação ira cancelar todos os itens desta OC, mesmo os já confirmados/faturados! Deseja continuar?", QMessageBox::Yes | QMessageBox::No,
                     this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not cancelar(list)) { return qApp->rollbackTransaction(); }

  if (not Sql::updateVendaStatus(idVendas.join(", "))) { return; }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Itens cancelados!");
}

void WidgetCompraFaturar::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!"); }

  InputDialog input(InputDialog::Tipo::Faturamento);
  if (input.exec() != InputDialog::Accepted) { return; }

  const QDate dataPrevista = input.getNextDate();

  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra AND status != 'CANCELADO' AND status != 'DEVOLVIDO'");

  for (const auto &item : list) {
    const int idCompra = modelViewFaturamento.data(item.row(), "idCompra").toInt();

    query1.bindValue(":dataPrevFat", dataPrevista);
    query1.bindValue(":idCompra", idCompra);

    if (not query1.exec()) { return qApp->enqueueError("Erro query pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":dataPrevFat", dataPrevista);
    query2.bindValue(":idCompra", idCompra);

    if (not query2.exec()) { return qApp->enqueueError("Erro query venda_has_produto: " + query2.lastError().text()); }
  }

  updateTables();

  qApp->enqueueInformation("Operação realizada com sucesso!");
}

// TODO: 4quando importar nota vincular com as contas_pagar
// TODO: 5reimportar nota id 4936 que veio com o produto dividido para testar o quantConsumido
// TODO: 5reestruturar na medida do possivel de forma que cada estoque tenha apenas uma nota/compra
// TODO: 0colocar tela de busca
