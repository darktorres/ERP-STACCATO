#include <QDate>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

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
  // REFAC: refactor this to not select in here

  modelResumo.setTable("view_fornecedor_compra_faturar");

  modelResumo.setFilter("(idVenda NOT LIKE '%CAMB%' OR idVenda IS NULL)");

  modelResumo.setHeaderData("fornecedor", "Forn.");

  ui->tableResumo->setModel(&modelResumo);
  ui->tableResumo->hideColumn("idVenda");

  model.setTable("view_faturamento");

  montaFiltro();

  //  model.setFilter("representacao = " + QString(ui->checkBoxRepresentacao->isChecked() ? "1" : "0"));

  model.setHeaderData("dataPrevFat", "Prev. Fat.");

  ui->table->setModel(&model);
  ui->table->setItemDelegateForColumn("Total", new ReaisDelegate(this));
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("representacao");

  connect(ui->checkBoxMostrarSul, &QCheckBox::toggled, this, &WidgetCompraFaturar::montaFiltro);
  connect(ui->checkBoxRepresentacao, &QCheckBox::toggled, this, &WidgetCompraFaturar::montaFiltro);
}

bool WidgetCompraFaturar::updateTables() {
  if (model.tableName().isEmpty()) setupTables();

  if (not modelResumo.select()) {
    emit errorSignal("Erro lendo tabela resumo: " + modelResumo.lastError().text());
    return false;
  }

  ui->tableResumo->resizeColumnsToContents();

  if (not model.select()) {
    emit errorSignal("Erro lendo tabela faturamento: " + model.lastError().text());
    return false;
  }

  ui->table->resizeColumnsToContents();

  return true;
}

bool WidgetCompraFaturar::faturarCompra(const QDateTime &dataReal, const QStringList &idsCompra) {
  QSqlQuery query;

  for (const auto &idCompra : idsCompra) {
    query.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM ENTREGA', dataRealFat = :dataRealFat WHERE idCompra = :idCompra");
    query.bindValue(":dataRealFat", dataReal);
    query.bindValue(":idCompra", idCompra);

    if (not query.exec()) {
      emit errorSignal("Erro atualizando status da compra: " + query.lastError().text());
      return false;
    }

    query.prepare("UPDATE venda_has_produto SET status = 'EM ENTREGA' WHERE idCompra = :idCompra");
    query.bindValue(":idCompra", idCompra);

    if (not query.exec()) {
      emit errorSignal("Erro atualizando status do produto da venda: " + query.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetCompraFaturar::on_pushButtonMarcarFaturado_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Não selecionou nenhuma compra!");
    return;
  }

  QStringList idsCompra;
  QStringList fornecedores;

  for (const auto &item : list) {
    idsCompra << model.data(item.row(), "idCompra").toString();
    fornecedores << model.data(item.row(), "fornecedor").toString();
  }

  const int size = fornecedores.size();

  if (fornecedores.removeDuplicates() != size - 1) {
    QMessageBox::critical(this, "Erro!", "Fornecedores diferentes!");
    return;
  }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::Faturamento);
  if (not inputDlg.setFilter(idsCompra)) return;
  if (inputDlg.exec() != InputDialogProduto::Accepted) return;

  const QDateTime dataReal = inputDlg.getDate();

  // TODO: 0quando a sigla CAMB pular

  const bool pularNota = ui->checkBoxRepresentacao->isChecked() or fornecedores.first() == "ATELIER" ? true : false;

  if (pularNota) {
    emit transactionStarted();

    QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
    QSqlQuery("START TRANSACTION").exec();

    if (not faturarCompra(dataReal, idsCompra)) {
      QSqlQuery("ROLLBACK").exec();
      emit transactionEnded();
      return;
    }

    QSqlQuery("COMMIT").exec();

    emit transactionEnded();
  } else {
    auto *import = new ImportarXML(idsCompra, dataReal, this);
    import->setAttribute(Qt::WA_DeleteOnClose);
    import->showMaximized();

    connect(import, &ImportarXML::errorSignal, this, &WidgetCompraFaturar::errorSignal);
    connect(import, &ImportarXML::transactionStarted, this, &WidgetCompraFaturar::transactionStarted);
    connect(import, &ImportarXML::transactionEnded, this, &WidgetCompraFaturar::transactionEnded);

    if (import->exec() != QDialog::Accepted) return;
  }

  QSqlQuery query;

  if (not query.exec("CALL update_venda_status()")) {
    QMessageBox::critical(this, "Erro!", "Erro atualizando status das vendas: " + query.lastError().text());
    return;
  }

  updateTables();
  QMessageBox::information(this, "Aviso!", "Confirmado faturamento!");
}

void WidgetCompraFaturar::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

// void WidgetCompraFaturar::on_checkBoxRepresentacao_toggled(bool checked) {
//  model.setFilter("representacao = " + QString(checked ? "1" : "0"));

//  if (not model.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + model.lastError().text());
//}

void WidgetCompraFaturar::montaFiltro() {
  const bool representacao = ui->checkBoxRepresentacao->isChecked();
  const bool sul = ui->checkBoxMostrarSul->isChecked();

  // REFAC: refactor 1/0 to true/false
  model.setFilter("representacao = " + QString(representacao ? "1" : "0") + " AND " + QString(sul ? "(Código LIKE 'CAMB%' OR Código IS NULL)" : "(Código NOT LIKE 'CAMB%' OR Código IS NULL)"));

  if (not model.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + model.lastError().text());
}

bool WidgetCompraFaturar::cancelar(const QModelIndexList &list) {
  // TODO: 0nas outras telas com cancelamento verificar se estou filtrando
  QSqlQuery query;

  for (const auto &item : list) {

    query.prepare("SELECT idVendaProduto FROM pedido_fornecedor_has_produto WHERE ordemCompra = :ordemCompra AND status = 'EM FATURAMENTO'");
    query.bindValue(":ordemCompra", model.data(item.row(), "OC"));

    if (not query.exec()) {
      emit errorSignal("Erro buscando dados: " + query.lastError().text());
      return false;
    }

    while (query.next()) {
      QSqlQuery query2;
      query2.prepare("UPDATE venda_has_produto SET status = 'PENDENTE', idCompra = NULL WHERE idVendaProduto = :idVendaProduto AND status = 'EM FATURAMENTO'");
      query2.bindValue(":idVendaProduto", query.value("idVendaProduto"));

      if (not query2.exec()) {
        emit errorSignal("Erro voltando status do produto: " + query2.lastError().text());
        return false;
      }
    }

    query.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'CANCELADO' WHERE ordemCompra = :ordemCompra AND status = 'EM FATURAMENTO'");
    query.bindValue(":ordemCompra", model.data(item.row(), "OC"));

    if (not query.exec()) {
      emit errorSignal("Erro salvando dados: " + query.lastError().text());
      return false;
    }

    // TODO: 5verificar como tratar isso
    //    query.prepare("UPDATE conta_a_pagar_has_pagamento SET status = 'CANCELADO' WHERE idCompra = :idCompra");
    //    query.bindValue(":idCompra", model.data(item.row(), "idCompra"));

    //    if (not query.exec()) {
    //      error = "Erro salvando pagamentos: " + query.lastError().text();
    //      return false;
    //    }
  }

  if (not query.exec("CALL update_venda_status()")) {
    emit errorSignal("Erro atualizando status da venda: " + query.lastError().text());
    return false;
  }

  // TODO: alterar a funcao de cancelar por uma tela de SAC onde o usuario indica as operacoes necessarias (troca de nfe, produto nao disponivel etc) e realiza as mudanças necessarias, bem como
  // alteracoes no fluxo de pagamento se necessario

  return true;
}

void WidgetCompraFaturar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Essa operação ira cancelar todos os itens desta OC, mesmo os já confirmados/faturados! Deseja continuar?", QMessageBox::Yes | QMessageBox::No,
                     this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) return;

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not cancelar(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  updateTables();
  QMessageBox::information(this, "Aviso!", "Itens cancelados!");
}

void WidgetCompraFaturar::on_pushButtonReagendar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  InputDialog input(InputDialog::Tipo::Faturamento);
  if (input.exec() != InputDialog::Accepted) return;

  const QDate dataPrevista = input.getNextDate();

  for (const auto &item : list) {
    const int idCompra = model.data(item.row(), "idCompra").toInt();

    QSqlQuery query;
    query.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");
    query.bindValue(":dataPrevFat", dataPrevista);
    query.bindValue(":idCompra", idCompra);

    if (not query.exec()) {
      QMessageBox::critical(this, "Erro!", "Erro query pedido_fornecedor: " + query.lastError().text());
      return;
    }

    query.prepare("UPDATE venda_has_produto SET dataPrevFat = :dataPrevFat WHERE idCompra = :idCompra");
    query.bindValue(":dataPrevFat", dataPrevista);
    query.bindValue(":idCompra", idCompra);

    if (not query.exec()) {
      QMessageBox::critical(this, "Erro!", "Erro query venda_has_produto: " + query.lastError().text());
      return;
    }
  }

  updateTables();

  QMessageBox::information(this, "Aviso!", "Operação realizada com sucesso!");
}

// TODO: 4quando importar nota vincular com as contas_pagar
// TODO: 5reimportar nota id 4936 que veio com o produto dividido para testar o quantConsumido
// TODO: 5reestruturar na medida do possivel de forma que cada estoque tenha apenas uma nota/compra
// TODO: 0colocar tela de busca
