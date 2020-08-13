#include "cadastropagamento.h"
#include "ui_cadastropagamento.h"

#include "application.h"
#include "checkboxdelegate.h"
#include "editdelegate.h"
#include "itemboxdelegate.h"
#include "porcentagemdelegate.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

CadastroPagamento::CadastroPagamento(QWidget *parent) : QDialog(parent), ui(new Ui::CadastroPagamento) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();
  setupMapper();

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
  ui->itemBoxContaDestino->setSearchDialog(SearchDialog::conta(this));

  ui->pushButtonAtualizarPagamento->hide();

  connect(ui->itemBoxLoja, &ItemBox::idChanged, this, &CadastroPagamento::on_itemBoxLoja_idChanged);
  connect(ui->pushButtonAdicionaAssociacao, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAdicionaAssociacao_clicked);
  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAdicionarPagamento_clicked);
  connect(ui->pushButtonAtualizarPagamento, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAtualizarPagamento_clicked);
  connect(ui->pushButtonAtualizarTaxas, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAtualizarTaxas_clicked);
  connect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonLimparSelecao_clicked);
  connect(ui->pushButtonRemoveAssociacao, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonRemoveAssociacao_clicked);
  connect(ui->pushButtonRemoverPagamento, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonRemoverPagamento_clicked);
  connect(ui->tablePagamentos, &TableView::clicked, this, &CadastroPagamento::on_tablePagamentos_clicked);
}

CadastroPagamento::~CadastroPagamento() { delete ui; }

void CadastroPagamento::setupTables() {
  modelPagamentos.setTable("forma_pagamento");

  modelPagamentos.setFilter("");

  modelPagamentos.setHeaderData("pagamento", "Pagamento");
  modelPagamentos.setHeaderData("parcelas", "Parcelas");
  modelPagamentos.setHeaderData("idConta", "Conta Destino");
  modelPagamentos.setHeaderData("pula1Mes", "Pula 1º Mês");
  modelPagamentos.setHeaderData("ajustaDiaUtil", "Ajusta Dia Útil");
  modelPagamentos.setHeaderData("dMaisUm", "D+1");
  modelPagamentos.setHeaderData("centavoSobressalente", "Centavo 1ª parcela");
  modelPagamentos.setHeaderData("apenasRepresentacao", "Apenas Representação");

  if (not modelPagamentos.select()) { return; }

  ui->tablePagamentos->setModel(&modelPagamentos);

  ui->tablePagamentos->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, true, this));
  ui->tablePagamentos->setItemDelegateForColumn("pula1Mes", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("ajustaDiaUtil", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("dMaisUm", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("centavoSobressalente", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("apenasRepresentacao", new CheckBoxDelegate(true, this));

  ui->tablePagamentos->setPersistentColumns({"idConta", "pula1Mes", "ajustaDiaUtil", "dMaisUm", "centavoSobressalente", "apenasRepresentacao"});

  ui->tablePagamentos->hideColumn("idPagamento");

  // -------------------------------------------------------------------------

  modelTaxas.setTable("forma_pagamento_has_taxa");

  modelTaxas.setHeaderData("taxa", "Taxa");

  ui->tableTaxas->setModel(&modelTaxas);

  ui->tableTaxas->hideColumn("idTaxa");
  ui->tableTaxas->hideColumn("idPagamento");
  ui->tableTaxas->hideColumn("parcela");

  ui->tableTaxas->setItemDelegateForColumn("taxa", new PorcentagemDelegate(false, this));

  // -------------------------------------------------------------------------

  modelAssocia1.setTable("forma_pagamento");

  modelAssocia1.setHeaderData("pagamento", "Pagamento");

  ui->tableAssocia1->setModel(&modelAssocia1);

  ui->tableAssocia1->hideColumn("idPagamento");
  ui->tableAssocia1->hideColumn("parcelas");
  ui->tableAssocia1->hideColumn("idConta");
  ui->tableAssocia1->hideColumn("pula1Mes");
  ui->tableAssocia1->hideColumn("ajustaDiaUtil");
  ui->tableAssocia1->hideColumn("dMaisUm");
  ui->tableAssocia1->hideColumn("centavoSobressalente");
  ui->tableAssocia1->hideColumn("apenasRepresentacao");

  // -------------------------------------------------------------------------

  modelAssocia2.setTable("view_pagamento_loja");

  modelAssocia2.setHeaderData("pagamento", "Pagamento");

  ui->tableAssocia2->setModel(&modelAssocia2);

  ui->tableAssocia2->hideColumn("idLoja");
  ui->tableAssocia2->hideColumn("idPagamento");
  ui->tableAssocia2->hideColumn("apenasRepresentacao");
}

void CadastroPagamento::updateTables() {
  if (not modelAssocia1.select()) { return; }

  if (not modelAssocia2.select()) { return; }
}

void CadastroPagamento::setupMapper() {
  mapperPagamento.setModel(&modelPagamentos);
  mapperPagamento.setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
  mapperPagamento.addMapping(ui->lineEditPagamento, modelPagamentos.fieldIndex("pagamento"));
  mapperPagamento.addMapping(ui->spinBoxParcelas, modelPagamentos.fieldIndex("parcelas"));
  mapperPagamento.addMapping(ui->itemBoxContaDestino, modelPagamentos.fieldIndex("idConta"), "id");
  mapperPagamento.addMapping(ui->checkBoxPula1Mes, modelPagamentos.fieldIndex("pula1Mes"));
  mapperPagamento.addMapping(ui->checkBoxDiaUtil, modelPagamentos.fieldIndex("ajustaDiaUtil"));
  mapperPagamento.addMapping(ui->checkBoxDMaisUm, modelPagamentos.fieldIndex("dMaisUm"));
  mapperPagamento.addMapping(ui->checkBoxCentavoSobressalente, modelPagamentos.fieldIndex("centavoSobressalente"));
  mapperPagamento.addMapping(ui->checkBoxApenasRepresentacao, modelPagamentos.fieldIndex("apenasRepresentacao"));
}

void CadastroPagamento::limparSelecao() {
  ui->lineEditPagamento->clear();
  ui->spinBoxParcelas->setValue(1);
  ui->itemBoxContaDestino->clear();
  ui->checkBoxPula1Mes->setChecked(false);
  ui->checkBoxDiaUtil->setChecked(false);
  ui->checkBoxDMaisUm->setChecked(false);
  ui->checkBoxCentavoSobressalente->setChecked(false);
  ui->checkBoxApenasRepresentacao->setChecked(false);

  //--------------------------------------

  ui->pushButtonAtualizarPagamento->hide();
  ui->pushButtonAdicionarPagamento->show();

  ui->pushButtonAtualizarTaxas->setDisabled(true);

  //--------------------------------------

  ui->tablePagamentos->clearSelection();

  modelTaxas.setFilter("0");
}

void CadastroPagamento::on_pushButtonAdicionarPagamento_clicked() {
  // TODO: nao deixar cadastrar pagamento com nome já existente pois dá erro depois no montarFluxo()

  if (ui->lineEditPagamento->text().isEmpty()) {
    qApp->enqueueError("Digite um nome para o pagamento!", this);
    ui->lineEditPagamento->setFocus();
    return;
  }

  if (not qApp->startTransaction("CadastroLoja::on_pushButtonAdicionarPagamento")) { return; }

  if (not adicionarPagamento()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
}

bool CadastroPagamento::adicionarPagamento() {
  const int row = modelPagamentos.insertRowAtEnd();

  if (not modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text())) { return false; }
  if (not modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value())) { return false; }
  if (not modelPagamentos.setData(row, "idConta", ui->itemBoxContaDestino->getId())) { return false; }
  if (not modelPagamentos.setData(row, "pula1Mes", ui->checkBoxPula1Mes->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "ajustaDiaUtil", ui->checkBoxDiaUtil->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "dMaisUm", ui->checkBoxDMaisUm->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "centavoSobressalente", ui->checkBoxCentavoSobressalente->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "apenasRepresentacao", ui->checkBoxApenasRepresentacao->isChecked())) { return false; }

  if (not modelPagamentos.submitAll()) { return false; }

  const int id = modelPagamentos.query().lastInsertId().toInt();

  for (int i = 0; i < ui->spinBoxParcelas->value(); ++i) {
    const int rowTaxas = modelTaxas.insertRowAtEnd();

    if (not modelTaxas.setData(rowTaxas, "idPagamento", id)) { return false; }
    if (not modelTaxas.setData(rowTaxas, "parcela", i + 1)) { return false; }
    if (not modelTaxas.setData(rowTaxas, "taxa", 0)) { return false; }
  }

  if (not modelTaxas.submitAll()) { return false; }

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  return modelTaxas.select();
}

void CadastroPagamento::on_pushButtonAtualizarPagamento_clicked() {
  if (not qApp->startTransaction("CadastroLoja::on_pushButtonAtualizarPagamento")) { return; }

  if (not atualizarPagamento()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
}

bool CadastroPagamento::atualizarPagamento() {
  const int row = mapperPagamento.currentIndex();

  if (not modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text())) { return false; }
  if (not modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value())) { return false; }
  if (not modelPagamentos.setData(row, "idConta", ui->itemBoxContaDestino->getId())) { return false; }
  if (not modelPagamentos.setData(row, "pula1Mes", ui->checkBoxPula1Mes->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "ajustaDiaUtil", ui->checkBoxDiaUtil->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "dMaisUm", ui->checkBoxDMaisUm->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "centavoSobressalente", ui->checkBoxCentavoSobressalente->isChecked())) { return false; }
  if (not modelPagamentos.setData(row, "apenasRepresentacao", ui->checkBoxApenasRepresentacao->isChecked())) { return false; }

  //--------------------------------------

  const int novaParcelas = ui->spinBoxParcelas->value();
  const int antigoParcelas = modelTaxas.rowCount();
  const int diferenca = qAbs(novaParcelas - antigoParcelas);

  if (novaParcelas < antigoParcelas) { // remove linhas
    for (int i = 0; i < diferenca; ++i) {
      if (not modelTaxas.removeRow(modelTaxas.rowCount() - 1 - i)) { return qApp->enqueueException(false, "Erro removendo linha: " + modelTaxas.lastError().text(), this); }
    }
  }

  if (novaParcelas > antigoParcelas) { // adiciona linhas
    for (int i = 0; i < diferenca; ++i) {
      const int rowTaxas = modelTaxas.insertRowAtEnd();

      if (not modelTaxas.setData(rowTaxas, "idPagamento", modelPagamentos.data(row, "idPagamento"))) { return false; }
      if (not modelTaxas.setData(rowTaxas, "parcela", modelTaxas.rowCount())) { return false; }
      if (not modelTaxas.setData(rowTaxas, "taxa", 0)) { return false; }
    }
  }

  //--------------------------------------

  if (not modelPagamentos.submitAll()) { return false; }

  if (not modelTaxas.submitAll()) { return false; }

  // submitAll resets currentIndex to -1, set it again
  mapperPagamento.setCurrentIndex(row);

  return true;
}

void CadastroPagamento::on_pushButtonRemoverPagamento_clicked() {
  const auto list = ui->tablePagamentos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  //--------------------------------------

  for (const auto &index : list) { modelPagamentos.removeRow(index.row()); }

  if (not modelPagamentos.submitAll()) { return qApp->enqueueException("Erro removendo pagamentos: " + modelPagamentos.lastError().text(), this); }

  if (not modelTaxas.select()) { return; }

  //--------------------------------------

  limparSelecao();
  updateTables();
}

void CadastroPagamento::on_pushButtonAdicionaAssociacao_clicked() {
  const auto list = ui->tableAssocia1->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  // -------------------------------------------------------------------------

  QSqlQuery query;
  query.prepare("INSERT INTO loja_has_forma_pagamento (idPagamento, idLoja) VALUES (:idPagamento, :idLoja)");

  for (const auto &index : list) {
    query.bindValue(":idPagamento", modelAssocia1.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", ui->itemBoxLoja->getId());

    if (not query.exec()) { qApp->enqueueException("Erro cadastrando associacao: " + query.lastError().text(), this); }
  }

  // -------------------------------------------------------------------------

  if (not modelAssocia1.select()) { qApp->enqueueException("Erro atualizando tabela: " + modelAssocia1.lastError().text(), this); }

  if (not modelAssocia2.select()) { qApp->enqueueException("Erro atualizando tabela: " + modelAssocia2.lastError().text(), this); }
}

void CadastroPagamento::on_pushButtonRemoveAssociacao_clicked() {
  const auto list = ui->tableAssocia2->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  // -------------------------------------------------------------------------

  QSqlQuery query;
  query.prepare("DELETE FROM loja_has_forma_pagamento WHERE idPagamento = :idPagamento AND idLoja = :idLoja");

  for (const auto &index : list) {
    query.bindValue(":idPagamento", modelAssocia2.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", modelAssocia2.data(index.row(), "idLoja"));

    if (not query.exec()) { qApp->enqueueException("Erro removendo associacao: " + query.lastError().text(), this); }
  }

  // -------------------------------------------------------------------------

  if (not modelAssocia1.select()) { qApp->enqueueException("Erro atualizando tabela: " + modelAssocia1.lastError().text(), this); }

  if (not modelAssocia2.select()) { qApp->enqueueException("Erro atualizando tabela: " + modelAssocia2.lastError().text(), this); }
}

void CadastroPagamento::on_pushButtonAtualizarTaxas_clicked() {
  if (not modelTaxas.submitAll()) { return; }

  qApp->enqueueInformation("Taxas atualizadas!", this);
}

void CadastroPagamento::on_tablePagamentos_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return limparSelecao(); }

  const int id = modelPagamentos.data(index.row(), "idPagamento").toInt();

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  if (not modelTaxas.select()) { return; }

  mapperPagamento.setCurrentModelIndex(index);

  ui->pushButtonAdicionarPagamento->hide();
  ui->pushButtonAtualizarPagamento->show();

  ui->pushButtonAtualizarTaxas->setEnabled(true);
}

void CadastroPagamento::on_itemBoxLoja_idChanged(const QVariant &id) {
  modelAssocia1.setFilter("idPagamento NOT IN (SELECT idPagamento FROM view_pagamento_loja WHERE idLoja = " + id.toString() + ")");

  if (not modelAssocia1.select()) { return; }

  // -------------------------------------------------------------------------

  modelAssocia2.setFilter("idLoja = " + id.toString());

  if (not modelAssocia2.select()) { return; }
}

void CadastroPagamento::on_pushButtonLimparSelecao_clicked() { limparSelecao(); }
