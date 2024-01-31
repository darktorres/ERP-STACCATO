#include "cadastropagamento.h"
#include "ui_cadastropagamento.h"

#include "application.h"
#include "checkboxdelegate.h"
#include "itemboxdelegate.h"
#include "porcentagemdelegate.h"
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>

CadastroPagamento::CadastroPagamento(QWidget *parent) : QDialog(parent), ui(new Ui::CadastroPagamento) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();
  setupMapper();

  ui->itemBoxLoja->setSearchDialog(SearchDialog::loja(this));
  ui->itemBoxContaDestino->setSearchDialog(SearchDialog::conta(this));

  ui->pushButtonAtualizarPagamento->hide();

  setConnections();

  ui->itemBoxContaLoja->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxContaProfissional->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxContaVendedor->setSearchDialog(SearchDialog::conta(this));

  ui->itemBoxContaLoja->setReadOnlyItemBox(true);
  ui->itemBoxContaProfissional->setReadOnlyItemBox(true);
  ui->itemBoxContaVendedor->setReadOnlyItemBox(true);

  ui->comboBoxLoja->addItem("");
  ui->comboBoxProfissional->addItem("");
  ui->comboBoxVendedor->addItem("");

  for (int row = 0; row < modelPagamentos.rowCount(); ++row) {
    ui->comboBoxLoja->addItem(modelPagamentos.data(row, "pagamento").toString(), modelPagamentos.data(row, "idPagamento").toInt());
    ui->comboBoxProfissional->addItem(modelPagamentos.data(row, "pagamento").toString(), modelPagamentos.data(row, "idPagamento").toInt());
    ui->comboBoxVendedor->addItem(modelPagamentos.data(row, "pagamento").toString(), modelPagamentos.data(row, "idPagamento").toInt());
  }

  SqlQuery query;

  if (not query.exec("SELECT contaComissaoLoja, contaComissaoProfissional, contaComissaoVendedor FROM config WHERE idConfig = 1")) {
    throw RuntimeException("Erro buscando contas padrão: " + query.lastError().text());
  }

  if (query.first()) {
    ui->comboBoxLoja->setCurrentIndex(ui->comboBoxLoja->findData(query.value("contaComissaoLoja").toInt()));
    ui->comboBoxProfissional->setCurrentIndex(ui->comboBoxProfissional->findData(query.value("contaComissaoProfissional").toInt()));
    ui->comboBoxVendedor->setCurrentIndex(ui->comboBoxVendedor->findData(query.value("contaComissaoVendedor").toInt()));
  }
}

CadastroPagamento::~CadastroPagamento() {
  delete ui;

  SearchDialog::clearCache();
}

void CadastroPagamento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->comboBoxLoja, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastroPagamento::on_comboBoxLoja_currentIndexChanged, connectionType);
  connect(ui->comboBoxProfissional, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastroPagamento::on_comboBoxProfissional_currentIndexChanged, connectionType);
  connect(ui->comboBoxVendedor, qOverload<int>(&QComboBox::currentIndexChanged), this, &CadastroPagamento::on_comboBoxVendedor_currentIndexChanged, connectionType);
  connect(ui->itemBoxLoja, &ItemBox::idChanged, this, &CadastroPagamento::on_itemBoxLoja_idChanged, connectionType);
  connect(ui->pushButtonAdicionaAssociacao, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAdicionaAssociacao_clicked, connectionType);
  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAdicionarPagamento_clicked, connectionType);
  connect(ui->pushButtonAtualizarPagamento, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAtualizarPagamento_clicked, connectionType);
  connect(ui->pushButtonAtualizarTaxas, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonAtualizarTaxas_clicked, connectionType);
  connect(ui->pushButtonLimparSelecao, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonLimparSelecao_clicked, connectionType);
  connect(ui->pushButtonRemoveAssociacao, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonRemoveAssociacao_clicked, connectionType);
  connect(ui->pushButtonRemoverPagamento, &QPushButton::clicked, this, &CadastroPagamento::on_pushButtonRemoverPagamento_clicked, connectionType);
  connect(ui->tablePagamentos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CadastroPagamento::on_tablePagamentos_selectionChanged, connectionType);
}

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

  modelPagamentos.select();

  ui->tablePagamentos->setModel(&modelPagamentos);

  ui->tablePagamentos->hideColumn("idPagamento");

  ui->tablePagamentos->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, true, this));
  ui->tablePagamentos->setItemDelegateForColumn("pula1Mes", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("ajustaDiaUtil", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("dMaisUm", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("centavoSobressalente", new CheckBoxDelegate(true, this));
  ui->tablePagamentos->setItemDelegateForColumn("apenasRepresentacao", new CheckBoxDelegate(true, this));

  ui->tablePagamentos->setPersistentColumns({"pula1Mes", "ajustaDiaUtil", "dMaisUm", "centavoSobressalente", "apenasRepresentacao"});

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
  modelAssocia1.select();

  modelAssocia2.select();
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

  modelTaxas.setFilter("0");

  ui->tablePagamentos->clearSelection();
}

void CadastroPagamento::on_pushButtonAdicionarPagamento_clicked() {
  // TODO: nao deixar cadastrar pagamento com nome já existente pois dá erro depois no montarFluxo()

  if (ui->lineEditPagamento->text().isEmpty()) { throw RuntimeError("Digite um nome para o pagamento!", this); }

  qApp->startTransaction("CadastroLoja::on_pushButtonAdicionarPagamento");

  adicionarPagamento();

  qApp->endTransaction();

  updateTables();
}

void CadastroPagamento::adicionarPagamento() {
  const int row = modelPagamentos.insertRowAtEnd();

  modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text());
  modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value());
  modelPagamentos.setData(row, "idConta", ui->itemBoxContaDestino->getId());
  modelPagamentos.setData(row, "pula1Mes", ui->checkBoxPula1Mes->isChecked());
  modelPagamentos.setData(row, "ajustaDiaUtil", ui->checkBoxDiaUtil->isChecked());
  modelPagamentos.setData(row, "dMaisUm", ui->checkBoxDMaisUm->isChecked());
  modelPagamentos.setData(row, "centavoSobressalente", ui->checkBoxCentavoSobressalente->isChecked());
  modelPagamentos.setData(row, "apenasRepresentacao", ui->checkBoxApenasRepresentacao->isChecked());

  modelPagamentos.submitAll();

  const int id = modelPagamentos.query().lastInsertId().toInt();

  for (int i = 0; i < ui->spinBoxParcelas->value(); ++i) {
    const int rowTaxas = modelTaxas.insertRowAtEnd();

    modelTaxas.setData(rowTaxas, "idPagamento", id);
    modelTaxas.setData(rowTaxas, "parcela", i + 1);
    modelTaxas.setData(rowTaxas, "taxa", 0);
  }

  modelTaxas.submitAll();

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  modelTaxas.select();
}

void CadastroPagamento::on_pushButtonAtualizarPagamento_clicked() {
  qApp->startTransaction("CadastroLoja::on_pushButtonAtualizarPagamento");

  atualizarPagamento();

  qApp->endTransaction();

  updateTables();
}

void CadastroPagamento::atualizarPagamento() {
  const int row = mapperPagamento.currentIndex();

  modelPagamentos.setData(row, "pagamento", ui->lineEditPagamento->text());
  modelPagamentos.setData(row, "parcelas", ui->spinBoxParcelas->value());
  modelPagamentos.setData(row, "idConta", ui->itemBoxContaDestino->getId());
  modelPagamentos.setData(row, "pula1Mes", ui->checkBoxPula1Mes->isChecked());
  modelPagamentos.setData(row, "ajustaDiaUtil", ui->checkBoxDiaUtil->isChecked());
  modelPagamentos.setData(row, "dMaisUm", ui->checkBoxDMaisUm->isChecked());
  modelPagamentos.setData(row, "centavoSobressalente", ui->checkBoxCentavoSobressalente->isChecked());
  modelPagamentos.setData(row, "apenasRepresentacao", ui->checkBoxApenasRepresentacao->isChecked());

  //--------------------------------------

  const int novaParcelas = ui->spinBoxParcelas->value();
  const int antigoParcelas = modelTaxas.rowCount();
  const int diferenca = qAbs(novaParcelas - antigoParcelas);

  if (novaParcelas < antigoParcelas) { // remove linhas
    for (int i = 0; i < diferenca; ++i) {
      if (not modelTaxas.removeRow(modelTaxas.rowCount() - 1 - i)) { throw RuntimeException("Erro removendo linha: " + modelTaxas.lastError().text()); }
    }
  }

  if (novaParcelas > antigoParcelas) { // adiciona linhas
    for (int i = 0; i < diferenca; ++i) {
      const int rowTaxas = modelTaxas.insertRowAtEnd();

      modelTaxas.setData(rowTaxas, "idPagamento", modelPagamentos.data(row, "idPagamento"));
      modelTaxas.setData(rowTaxas, "parcela", modelTaxas.rowCount());
      modelTaxas.setData(rowTaxas, "taxa", 0);
    }
  }

  //--------------------------------------

  modelPagamentos.submitAll();

  modelTaxas.submitAll();

  // submitAll resets currentIndex to -1, set it again
  mapperPagamento.setCurrentIndex(row);
}

void CadastroPagamento::on_pushButtonRemoverPagamento_clicked() {
  const auto selection = ui->tablePagamentos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  //--------------------------------------

  for (const auto &index : selection) { modelPagamentos.removeRow(index.row()); }

  modelPagamentos.submitAll();

  modelTaxas.select();

  //--------------------------------------

  limparSelecao();
  updateTables();
}

void CadastroPagamento::on_pushButtonAdicionaAssociacao_clicked() {
  const auto selection = ui->tableAssocia1->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  // -------------------------------------------------------------------------

  SqlQuery query;
  query.prepare("INSERT INTO loja_has_forma_pagamento (idPagamento, idLoja) VALUES (:idPagamento, :idLoja)");

  for (const auto &index : selection) {
    query.bindValue(":idPagamento", modelAssocia1.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", ui->itemBoxLoja->getId());

    if (not query.exec()) { throw RuntimeException("Erro cadastrando associacao: " + query.lastError().text(), this); }
  }

  // -------------------------------------------------------------------------

  modelAssocia1.select();

  modelAssocia2.select();
}

void CadastroPagamento::on_pushButtonRemoveAssociacao_clicked() {
  const auto selection = ui->tableAssocia2->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  // -------------------------------------------------------------------------

  SqlQuery query;
  query.prepare("DELETE FROM loja_has_forma_pagamento WHERE idPagamento = :idPagamento AND idLoja = :idLoja");

  for (const auto &index : selection) {
    query.bindValue(":idPagamento", modelAssocia2.data(index.row(), "idPagamento"));
    query.bindValue(":idLoja", modelAssocia2.data(index.row(), "idLoja"));

    if (not query.exec()) { throw RuntimeException("Erro removendo associacao: " + query.lastError().text(), this); }
  }

  // -------------------------------------------------------------------------

  modelAssocia1.select();

  modelAssocia2.select();
}

void CadastroPagamento::on_pushButtonAtualizarTaxas_clicked() {
  modelTaxas.submitAll();

  qApp->enqueueInformation("Taxas atualizadas!", this);
}

void CadastroPagamento::on_tablePagamentos_selectionChanged() {
  const auto selection = ui->tablePagamentos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return limparSelecao(); }

  const auto index = selection.first();

  const int id = modelPagamentos.data(index.row(), "idPagamento").toInt();

  modelTaxas.setFilter("idPagamento = " + QString::number(id));

  modelTaxas.select();

  mapperPagamento.setCurrentModelIndex(index);

  ui->pushButtonAdicionarPagamento->hide();
  ui->pushButtonAtualizarPagamento->show();

  ui->pushButtonAtualizarTaxas->setEnabled(true);
}

void CadastroPagamento::on_itemBoxLoja_idChanged(const QVariant &id) {
  modelAssocia1.setFilter("idPagamento NOT IN (SELECT idPagamento FROM view_pagamento_loja WHERE idLoja = " + id.toString() + ")");

  modelAssocia1.select();

  // -------------------------------------------------------------------------

  modelAssocia2.setFilter("idLoja = " + id.toString());

  modelAssocia2.select();
}

void CadastroPagamento::on_pushButtonLimparSelecao_clicked() { limparSelecao(); }

void CadastroPagamento::on_comboBoxLoja_currentIndexChanged(const int index)
{
  Q_UNUSED(index)

  const int idPagamento = ui->comboBoxLoja->currentData().toInt();
  const auto match = modelPagamentos.match("idPagamento", idPagamento, 1, Qt::MatchExactly);

  if (not match.isEmpty()) {
    ui->itemBoxContaLoja->setId(modelPagamentos.data(match.first().row(), "idConta").toInt());

    SqlQuery query;

    if (not query.exec("UPDATE config SET contaComissaoLoja = " + QString::number(idPagamento) + " WHERE idConfig = 1")) {
      throw RuntimeException("Erro atualizando conta: " + query.lastError().text());
    }
  }
}

void CadastroPagamento::on_comboBoxProfissional_currentIndexChanged(const int index)
{
  Q_UNUSED(index)

  const int idPagamento = ui->comboBoxProfissional->currentData().toInt();
  const auto match = modelPagamentos.match("idPagamento", idPagamento, 1, Qt::MatchExactly);

  if (not match.isEmpty()) {
    ui->itemBoxContaProfissional->setId(modelPagamentos.data(match.first().row(), "idConta").toInt());

    SqlQuery query;

    if (not query.exec("UPDATE config SET contaComissaoProfissional = " + QString::number(idPagamento) + " WHERE idConfig = 1")) {
      throw RuntimeException("Erro atualizando conta: " + query.lastError().text());
    }
  }
}

void CadastroPagamento::on_comboBoxVendedor_currentIndexChanged(const int index)
{
  Q_UNUSED(index)

  const int idPagamento = ui->comboBoxVendedor->currentData().toInt();
  const auto match = modelPagamentos.match("idPagamento", idPagamento, 1, Qt::MatchExactly);

  if (not match.isEmpty()) {
    ui->itemBoxContaVendedor->setId(modelPagamentos.data(match.first().row(), "idConta").toInt());

    SqlQuery query;

    if (not query.exec("UPDATE config SET contaComissaoVendedor = " + QString::number(idPagamento) + " WHERE idConfig = 1")) {
      throw RuntimeException("Erro atualizando conta: " + query.lastError().text());
    }
  }
}

// TODO: colocar caixa de confirmacao antes de remover qualquer coisa
